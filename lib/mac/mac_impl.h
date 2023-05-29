/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "mac_config.h"
#include "mac_ctrl/mac_controller.h"
#include "mac_ctrl/mac_scheduler_configurator.h"
#include "mac_dl/mac_dl_processor.h"
#include "mac_dl/rlf_detector.h"
#include "mac_scheduler_adapter.h"
#include "mac_ul/mac_ul_processor.h"
#include "rach_handler/rach_handler.h"
#include "rach_handler/rnti_manager.h"
#include "srsran/mac/mac.h"
#include "srsran/mac/mac_config.h"
#include "srsran/mac/mac_paging_information_handler.h"
#include "srsran/scheduler/mac_scheduler.h"
#include "srsran/srslog/srslog.h"

namespace srsran {

class mac_impl : public mac_interface, public mac_ue_control_information_handler, public mac_paging_information_handler
{
public:
  explicit mac_impl(const mac_config& mac_cfg);

  mac_cell_rach_handler& get_rach_handler(du_cell_index_t cell_index) override { return rach_hdl.get_cell(cell_index); }

  mac_ue_configurator& get_ue_configurator() override { return ctrl_unit; }

  mac_cell_control_information_handler& get_control_info_handler(du_cell_index_t cell_index) override
  {
    return dl_unit.get_cell_control_information_handler(cell_index);
  }

  mac_ue_control_information_handler& get_ue_control_info_handler() override { return *this; }

  mac_cell_slot_handler& get_slot_handler(du_cell_index_t cell_index) override
  {
    return dl_unit.get_slot_handler(cell_index);
  }

  mac_cell_manager& get_cell_manager() override { return ctrl_unit; }

  mac_pdu_handler& get_pdu_handler(du_cell_index_t cell_index) override { return ul_unit; }

  void handle_dl_buffer_state_update_required(const mac_dl_buffer_state_indication_message& dl_bs) override;

  mac_paging_information_handler& get_cell_paging_info_handler() override { return *this; }

  void handle_paging_information(const paging_information& msg) override
  {
    sched_paging_information pg_info{};
    pg_info.paging_drx              = msg.paging_drx;
    pg_info.paging_identity         = msg.paging_identity;
    pg_info.paging_type_indicator   = msg.paging_type_indicator;
    pg_info.ue_identity_index_value = msg.ue_identity_index_value;
    pg_info.paging_cells.resize(msg.paging_cells.size());
    pg_info.paging_cells.assign(msg.paging_cells.begin(), msg.paging_cells.end());

    sched_obj->handle_paging_information(pg_info);
  }

private:
  mac_common_config_t cfg;

  /// Table used to convert from RNTI to UE index.
  rnti_manager rnti_table;

  /// Detector of UE RLFs.
  rlf_detector rlf_handler;

  /// Handle used to await scheduler configurations.
  std::unique_ptr<mac_scheduler_adapter> sched_cfg_adapter;

  /// MAC Scheduler.
  std::unique_ptr<mac_scheduler> sched_obj;

  mac_dl_processor dl_unit;
  mac_ul_processor ul_unit;
  rach_handler     rach_hdl;
  mac_controller   ctrl_unit;
};

} // namespace srsran
