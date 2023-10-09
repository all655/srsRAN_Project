/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */
#include "du_manager_procedure_test_helpers.h"
#include "lib/du_manager/procedures/ue_creation_procedure.h"
#include "lib/du_manager/procedures/ue_deletion_procedure.h"
#include "srsran/du/du_cell_config_helpers.h"
#include "srsran/support/test_utils.h"
#include <gtest/gtest.h>

using namespace srsran;
using namespace srs_du;

class ue_deletion_tester : public du_manager_proc_tester, public ::testing::Test
{
protected:
  ue_deletion_tester() :
    du_manager_proc_tester(std::vector<du_cell_config>{config_helpers::make_default_du_cell_config()})
  {
    // Create UE.
    test_ue = &create_ue(to_du_ue_index(test_rgen::uniform_int<unsigned>(0, MAX_DU_UE_INDEX)));

    // Run UE Configuration Procedure to completion.
    configure_ue(create_f1ap_ue_context_update_request(test_ue->ue_index, {srb_id_t::srb2}, {drb_id_t::drb1}));
  }

  void start_procedure()
  {
    f1ap_ue_delete_request msg;
    msg.ue_index = test_ue->ue_index;
    proc         = launch_async<ue_deletion_procedure>(msg, ue_mng, params);
    proc_launcher.emplace(proc);
  }

  du_ue*                             test_ue = nullptr;
  async_task<void>                   proc;
  optional<lazy_task_launcher<void>> proc_launcher;
};

TEST_F(ue_deletion_tester, when_du_manager_receives_ue_delete_request_then_f1ap_and_mac_get_request_to_delete_ue)
{
  start_procedure();

  // UE deletion started and completed in F1AP.
  ASSERT_TRUE(this->f1ap.last_ue_deleted.has_value());
  ASSERT_EQ(this->f1ap.last_ue_deleted.value(), test_ue->ue_index);

  // Check MAC received request to delete UE with valid params.
  ASSERT_TRUE(this->mac.last_ue_delete_msg.has_value());
  EXPECT_EQ(this->mac.last_ue_delete_msg->ue_index, test_ue->ue_index);
}

TEST_F(ue_deletion_tester,
       when_du_manager_starts_ue_deletion_procedure_then_it_waits_for_mac_completion_before_finishing_procedure)
{
  start_procedure();

  // Check MAC received request to delete UE but DU manager is waiting for MAC completion before deleting UE from F1AP.
  ASSERT_TRUE(this->mac.last_ue_delete_msg.has_value());
  ASSERT_FALSE(proc.ready());

  // MAC returns response to delete UE.
  // Note: UE is going to be removed, so we save its index locally.
  this->mac.wait_ue_delete.result.result = true;
  this->mac.wait_ue_delete.ready_ev.set();

  // UE deletion procedure should have finished at this point.
  ASSERT_TRUE(proc.ready());
}

TEST_F(ue_deletion_tester, when_du_manager_is_removing_ue_from_mac_then_rlc_buffer_states_have_no_effect)
{
  // RLC buffer state updates of DRBs should reach the MAC.
  ASSERT_FALSE(mac.last_dl_bs.has_value());
  test_ue->bearers.drbs().at(drb_id_t::drb1)->connector.rlc_tx_buffer_state_notif.on_buffer_state_update(10);
  ASSERT_TRUE(mac.last_dl_bs.has_value());
  ASSERT_EQ(mac.last_dl_bs->ue_index, test_ue->ue_index);
  ASSERT_EQ(mac.last_dl_bs->lcid, lcid_t::LCID_MIN_DRB);
  ASSERT_EQ(mac.last_dl_bs->bs, 10);

  // RLC buffer state updates of SRBs should reach the MAC.
  mac.last_dl_bs.reset();
  ASSERT_FALSE(mac.last_dl_bs.has_value());
  test_ue->bearers.srbs()[srb_id_t::srb1].connector.rlc_tx_buffer_state_notif.on_buffer_state_update(100);
  ASSERT_TRUE(mac.last_dl_bs.has_value());
  ASSERT_EQ(mac.last_dl_bs->ue_index, test_ue->ue_index);
  ASSERT_EQ(mac.last_dl_bs->lcid, LCID_SRB1);
  ASSERT_EQ(mac.last_dl_bs->bs, 100);

  start_procedure();

  // RLC buffer state updates should not reach the MAC while UE is being removed.
  mac.last_dl_bs.reset();
  test_ue->bearers.drbs().at(drb_id_t::drb1)->connector.rlc_tx_buffer_state_notif.on_buffer_state_update(10);
  ASSERT_FALSE(mac.last_dl_bs.has_value());
  test_ue->bearers.srbs()[srb_id_t::srb1].connector.rlc_tx_buffer_state_notif.on_buffer_state_update(10);
  ASSERT_FALSE(mac.last_dl_bs.has_value());
}

TEST_F(ue_deletion_tester, when_du_manager_is_removing_ue_from_mac_then_rlf_notifications_have_no_effect)
{
  // MAC RLF notification should be handled.
  test_ue->rlf_notifier->on_rlf_detected();
  ASSERT_EQ(ue_mng.last_rlf_ue_index, test_ue->ue_index);
  ASSERT_EQ(ue_mng.last_rlf_cause, rlf_cause::max_mac_kos_reached);

  // RLC RLF notification should be handled.
  test_ue->rlf_notifier->on_max_retx();
  ASSERT_EQ(ue_mng.last_rlf_ue_index, test_ue->ue_index);
  ASSERT_EQ(ue_mng.last_rlf_cause, rlf_cause::max_rlc_retxs_reached);

  // RLC RLF notification should be handled.
  test_ue->rlf_notifier->on_protocol_failure();
  ASSERT_EQ(ue_mng.last_rlf_ue_index, test_ue->ue_index);
  ASSERT_EQ(ue_mng.last_rlf_cause, rlf_cause::rlc_protocol_failure);

  // Start UE deletion.
  ue_mng.last_rlf_ue_index.reset();
  ue_mng.last_rlf_cause.reset();
  start_procedure();

  // RLF notifications should not be handled.
  test_ue->rlf_notifier->on_rlf_detected();
  ASSERT_FALSE(ue_mng.last_rlf_cause.has_value());
  ASSERT_FALSE(ue_mng.last_rlf_ue_index.has_value());
  test_ue->rlf_notifier->on_max_retx();
  ASSERT_FALSE(ue_mng.last_rlf_cause.has_value());
  ASSERT_FALSE(ue_mng.last_rlf_ue_index.has_value());
  test_ue->rlf_notifier->on_protocol_failure();
  ASSERT_FALSE(ue_mng.last_rlf_cause.has_value());
  ASSERT_FALSE(ue_mng.last_rlf_ue_index.has_value());
}
