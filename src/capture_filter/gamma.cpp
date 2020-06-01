/**
 * @file   capture_filter/gamma.cpp
 * @author Martin Pulec     <pulec@cesnet.cz>
 */
/*
 * Copyright (c) 2020 CESNET, z. s. p. o.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of CESNET nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "config_unix.h"
#include "config_win32.h"
#endif /* HAVE_CONFIG_H */

#include <iostream>
#include <stdexcept>
#include <vector>

#include "capture_filter.h"
#include "debug.h"
#include "lib_common.h"
#include "rang.hpp"
#include "utils/color_out.h"
#include "video.h"
#include "video_codec.h"

constexpr const char *MOD_NAME = "[gamma cap. f.] ";

using std::cout;
using std::exception;
using std::numeric_limits;
using rang::style;
using std::vector;

struct state_capture_filter_gamma {
public:
        explicit state_capture_filter_gamma(double gamma) {
                for (int i = 0; i <= numeric_limits<uint8_t>::max(); ++i) {
                        lut8.push_back(pow(static_cast<double>(i)
                                        / numeric_limits<uint8_t>::max(), gamma)
                                * numeric_limits<uint8_t>::max());
                }
                for (int i = 0; i < numeric_limits<uint16_t>::max(); ++i) {
                        lut16.push_back(pow(static_cast<double>(i)
                                        / numeric_limits<uint16_t>::max(), gamma)
                                * numeric_limits<uint16_t>::max());
                }
        }

        void apply_gamma(int depth, size_t len, void const *in, void *out) {
                if (depth == CHAR_BIT) {
                        apply_lut<uint8_t>(len,  lut8, in, out);
                } else if (depth == 2 * CHAR_BIT) {
                        apply_lut<uint16_t>(len,  lut16, in, out);
                } else {
                        throw exception();
                }
        }

private:
        template<typename T> void apply_lut(size_t len, const vector<T> &lut, void const *in, void *out)
        {
                auto *in_data = static_cast<const T*>(in);
                auto *out_data = static_cast<T*>(out);
                for (size_t i = 0; i < len; i += sizeof(T)) {
                        *out_data++ = lut[*in_data++];
                }
        }

        vector<uint8_t>  lut8;
        vector<uint16_t> lut16;
};

static auto init(struct module *parent, const char *cfg, void **state)
{
        UNUSED(parent);

        if (cfg == nullptr || strcmp(cfg, "help") == 0) {
                cout << "Performs gamma transformation.\n\n"
                       "usage:\n";
                cout << style::bold << "\t--capture-filter gamma:value\n" << style::reset;
                return 1;
        }
        char *endptr = nullptr;
        errno = 0;
        double gamma = strtod(cfg, &endptr);

        if (gamma <= 0.0 || errno != 0 || *endptr != '\0') {
                LOG(LOG_LEVEL_WARNING) << MOD_NAME << "Using gamma value " << gamma << "\n";
        }

        auto *s = new state_capture_filter_gamma(gamma);

        *state = s;
        return 0;
}

static void done(void *state)
{
        delete static_cast<state_capture_filter_gamma *>(state);
}

static auto filter(void *state, struct video_frame *in)
{
        auto *s = static_cast<state_capture_filter_gamma *>(state);
        struct video_desc desc = video_desc_from_frame(in);
        struct video_frame *out = vf_alloc_desc_data(desc);
        out->callbacks.dispose = vf_free;

        try {
                s->apply_gamma(get_bits_per_component(desc.color_spec), in->tiles[0].data_len, in->tiles[0].data, out->tiles[0].data);
        } catch(...) {
                LOG(LOG_LEVEL_ERROR) << MOD_NAME << "Only 8-bit and 16-bit codecs are currently supported!\n";
                vf_free(out);
                out = nullptr;
        }

        VIDEO_FRAME_DISPOSE(in);

        return out;
}

static const struct capture_filter_info capture_filter_gamma = {
        .init = init,
        .done = done,
        .filter = filter,
};

REGISTER_MODULE(gamma, &capture_filter_gamma, LIBRARY_CLASS_CAPTURE_FILTER, CAPTURE_FILTER_ABI_VERSION);

/* vim: set expandtab sw=8: */
