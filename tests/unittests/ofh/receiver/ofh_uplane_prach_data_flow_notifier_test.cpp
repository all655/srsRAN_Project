/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "../../../../lib/ofh/receiver/ofh_uplane_prach_data_flow_notifier.h"
#include "../ofh_uplane_rx_symbol_notifier_test_doubles.h"
#include "helpers.h"
#include <gtest/gtest.h>

using namespace srsran;
using namespace ofh;
using namespace ofh::testing;

TEST(ofh_uplane_prach_data_flow_notifier, empty_context_does_not_notify)
{
  auto notifier = std::make_shared<uplane_rx_symbol_notifier_spy>();
  auto repo     = std::make_shared<prach_context_repository>(1, srslog::fetch_basic_logger("TEST"));
  uplane_prach_data_flow_notifier sender(srslog::fetch_basic_logger("TEST"), repo, notifier);
  slot_point                      slot(0, 0, 1);

  sender.notify_prach(slot);

  ASSERT_TRUE(repo->get(slot).empty());
  ASSERT_FALSE(notifier->has_new_uplink_symbol_function_been_called());
  ASSERT_FALSE(notifier->has_new_prach_function_been_called());
}

TEST(ofh_uplane_prach_data_flow_notifier, unwritten_buffer_does_not_notify)
{
  auto notifier = std::make_shared<uplane_rx_symbol_notifier_spy>();
  auto repo     = std::make_shared<prach_context_repository>(1, srslog::fetch_basic_logger("TEST"));
  uplane_prach_data_flow_notifier sender(srslog::fetch_basic_logger("TEST"), repo, notifier);
  slot_point                      slot(0, 0, 1);
  prach_buffer_dummy              buffer(1);
  prach_buffer_context            context;
  context.slot             = slot;
  context.format           = prach_format_type::zero;
  context.ports            = {0};
  context.nof_td_occasions = 1;
  context.nof_fd_occasions = 1;
  context.pusch_scs        = srsran::subcarrier_spacing::kHz30;

  repo->add(context, buffer);
  sender.notify_prach(slot);

  ASSERT_FALSE(repo->get(slot).empty());
  ASSERT_FALSE(notifier->has_new_uplink_symbol_function_been_called());
  ASSERT_FALSE(notifier->has_new_prach_function_been_called());
}

TEST(ofh_uplane_prach_data_flow_notifier, completed_long_prach_buffer_triggers_notification)
{
  auto notifier = std::make_shared<uplane_rx_symbol_notifier_spy>();
  auto repo     = std::make_shared<prach_context_repository>(1, srslog::fetch_basic_logger("TEST"));
  uplane_prach_data_flow_notifier sender(srslog::fetch_basic_logger("TEST"), repo, notifier);
  slot_point                      slot(0, 0, 1);
  unsigned                        symbol = 0;
  unsigned                        port   = 0;
  prach_buffer_dummy              buffer(1);
  prach_buffer_context            context;
  context.slot             = slot;
  context.format           = prach_format_type::zero;
  context.ports            = {0};
  context.nof_td_occasions = 1;
  context.nof_fd_occasions = 1;
  context.pusch_scs        = srsran::subcarrier_spacing::kHz30;

  static_vector<cf_t, 839> samples(839);
  repo->add(context, buffer);
  ASSERT_FALSE(repo->get(slot).empty());

  // Fill the grid.
  repo->write_iq(slot, port, symbol, 0, samples);

  sender.notify_prach(slot);

  // Assert that the context is cleared when it is notified.
  ASSERT_TRUE(repo->get(slot).empty());

  // Assert the notification.
  ASSERT_FALSE(notifier->has_new_uplink_symbol_function_been_called());
  ASSERT_TRUE(notifier->has_new_prach_function_been_called());
}

TEST(ofh_uplane_prach_data_flow_notifier, completed_short_prach_buffer_triggers_notification)
{
  auto notifier = std::make_shared<uplane_rx_symbol_notifier_spy>();
  auto repo     = std::make_shared<prach_context_repository>(1, srslog::fetch_basic_logger("TEST"));
  uplane_prach_data_flow_notifier sender(srslog::fetch_basic_logger("TEST"), repo, notifier);
  slot_point                      slot(0, 0, 1);
  unsigned                        port = 0;
  prach_buffer_dummy              buffer(1, false);
  prach_buffer_context            context;
  context.slot             = slot;
  context.format           = prach_format_type::B4;
  context.ports            = {0};
  context.nof_td_occasions = 1;
  context.nof_fd_occasions = 1;
  context.pusch_scs        = srsran::subcarrier_spacing::kHz30;

  static_vector<cf_t, 139> samples(139);
  repo->add(context, buffer);
  ASSERT_FALSE(repo->get(slot).empty());

  // Fill the grid.
  for (unsigned i = 0; i != 12; ++i) {
    repo->write_iq(slot, port, i, 0, samples);
  }

  sender.notify_prach(slot);

  // Assert that the context is cleared when it is notified.
  ASSERT_TRUE(repo->get(slot).empty());

  // Assert the notification.
  ASSERT_FALSE(notifier->has_new_uplink_symbol_function_been_called());
  ASSERT_TRUE(notifier->has_new_prach_function_been_called());
}
