/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsran/adt/static_vector.h"
#include "srsran/ran/phy_time_unit.h"
#include "srsran/ran/prach/prach_constants.h"
#include "srsran/ran/slot_pdu_capacity_constants.h"
#include "srsran/ran/slot_point.h"

namespace srsran {

/// Describes a RACH indication.
struct mac_rach_indication {
  /// Describes the detection of a single preamble.
  struct rach_preamble {
    /// Index of the detected preamble. Possible values are {0, ..., 63}.
    unsigned index;
    /// Timing advance between the observed arrival time (for the considered UE) and the reference uplink time.
    phy_time_unit time_advance;
    /// Preamble received power in dBFS.
    optional<float> pwr_dBFS;
    /// Average SNR value in dB.
    optional<float> snr_dB;
  };

  /// Describes a single RACH occasion.
  struct rach_occasion {
    /// OFDM symbol index within the slot that marks the start of the acquisition window for the first time-domain PRACH
    /// occasion.
    unsigned start_symbol;
    /// The index of first slot of the PRACH TD occasion in a system frame.
    unsigned slot_index;
    /// The index of the received PRACH frequency domain occasion.
    unsigned frequency_index;
    /// Average value of RSSI in dBFS.
    optional<float> rssi_dBFS;
    /// List of detected preambles in this RACH occasion.
    static_vector<rach_preamble, MAX_PREAMBLES_PER_PRACH_OCCASION> preambles;
  };

  /// Slot point corresponding to the reception of this indication.
  slot_point slot_rx;
  /// List of RACH occasions carried in this indication.
  static_vector<rach_occasion, MAX_PRACH_OCCASIONS_PER_SLOT> occasions;
};

/// Interface used to handle RACH indications specific to a cell.
class mac_cell_rach_handler
{
public:
  virtual ~mac_cell_rach_handler() = default;

  /// \brief Handles incoming RACH indication from the L1.
  ///
  /// \param rach_ind Received RACH indication.
  virtual void handle_rach_indication(const mac_rach_indication& rach_ind) = 0;
};

} // namespace srsran
