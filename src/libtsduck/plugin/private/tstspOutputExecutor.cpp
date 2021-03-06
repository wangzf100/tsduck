//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tstspOutputExecutor.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tsp::OutputExecutor::OutputExecutor(const TSProcessorArgs& options,
                                        const PluginOptions& pl_options,
                                        const ThreadAttributes& attributes,
                                        Mutex& global_mutex,
                                        Report* report) :

    PluginExecutor(options, OUTPUT_PLUGIN, pl_options, attributes, global_mutex, report),
    _output(dynamic_cast<OutputPlugin*>(PluginThread::plugin()))
{
}


//----------------------------------------------------------------------------
// Output plugin thread
//----------------------------------------------------------------------------

void ts::tsp::OutputExecutor::main()
{
    debug(u"output thread started");

    PacketCounter output_packets = 0;
    bool aborted = false;

    do {
        // Wait for packets to output
        size_t pkt_first = 0;
        size_t pkt_cnt = 0;
        bool input_end = false;
        bool timeout = false;
        waitWork(pkt_first, pkt_cnt, _tsp_bitrate, input_end, aborted, timeout);

        // We ignore the returned "aborted" which comes from the "next"
        // processor in the chain, here the input thread. For the
        // output thread, aborted means was interrupted by user.
        aborted = _tsp_aborting;

        // Process restart requests.
        if (!processPendingRestart()) {
            timeout = true;
        }

        // In case of abort on timeout, notify previous and next plugin, then exit.
        if (timeout) {
            // Do not transmit bitrate or input end to next (since next is input processor).
            passPackets(0, 0, false, true);
            aborted = true;
            break;
        }

        // Exit thread if no more packet to process
        if ((pkt_cnt == 0 && input_end) || aborted) {
            break;
        }

        // Check if "joint termination" agreed on a last packet to output
        const PacketCounter jt_limit = totalPacketsBeforeJointTermination();
        if (totalPacketsInThread() + pkt_cnt > jt_limit) {
            pkt_cnt = totalPacketsInThread() > jt_limit ? 0 : size_t (jt_limit - totalPacketsInThread());
            aborted = true;
        }

        // Output the packets. Output may be segmented if dropped packets
        // (ie. starting with a zero byte) are in the middle of the buffer.

        TSPacket* pkt = _buffer->base() + pkt_first;
        TSPacketMetadata* data = _metadata->base() + pkt_first;
        size_t pkt_remain = pkt_cnt;

        while (pkt_remain > 0) {

            // Skip dropped packets
            size_t drop_cnt;
            for (drop_cnt = 0; drop_cnt < pkt_remain && pkt[drop_cnt].b[0] == 0; drop_cnt++) {}

            pkt += drop_cnt;
            data += drop_cnt;
            pkt_remain -= drop_cnt;
            addNonPluginPackets(drop_cnt);

            // Find last non-dropped packet
            size_t out_cnt = 0;
            while (out_cnt < pkt_remain && pkt[out_cnt].b[0] != 0) {
                out_cnt++;
            }

            // Output a contiguous range of non-dropped packets.
            if (out_cnt > 0) {
                if (_suspended) {
                    // Don't output packet when the plugin is suspended.
                    addNonPluginPackets(out_cnt);
                }
                else if (_output->send(pkt, data, out_cnt)) {
                    // Packet successfully sent.
                    addPluginPackets(out_cnt);
                    output_packets += out_cnt;
                }
                else {
                    // Send error.
                    aborted = true;
                    break;
                }
                pkt += out_cnt;
                data += out_cnt;
                pkt_remain -= out_cnt;
            }
        }

        // Pass free buffers to input processor.
        // Do not transmit bitrate or input end to next (since next is input processor).
        aborted = !passPackets(pkt_cnt, 0, false, aborted);

    } while (!aborted);

    // Close the output processor
    _output->stop();

    debug(u"output thread %s after %'d packets (%'d output)", {aborted ? u"aborted" : u"terminated", totalPacketsInThread(), output_packets});
}
