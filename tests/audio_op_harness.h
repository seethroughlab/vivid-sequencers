#pragma once

#include "runtime/operator_registry.h"
#include <cstring>
#include <string>
#include <vector>

struct AudioOpHarness {
    vivid::OperatorLoader* loader = nullptr;
    void* instance = nullptr;
    const VividOperatorDescriptor* desc = nullptr;
    std::vector<float> params;
    std::vector<float> input_floats;
    std::vector<float> output_floats;
    std::vector<VividSpreadPort> input_spreads;
    std::vector<VividSpreadPort> output_spreads;
    std::vector<std::vector<float>> input_spread_storage;
    std::vector<std::vector<float>> output_spread_storage;
    std::vector<void*> custom_inputs;
    std::vector<void*> custom_outputs;
    std::vector<std::string> file_params;
    std::vector<const char*> file_param_ptrs;

    explicit AudioOpHarness(vivid::OperatorLoader* in_loader, uint32_t spread_capacity = 32)
        : loader(in_loader) {
        desc = loader ? loader->descriptor() : nullptr;
        instance = loader ? loader->create_instance() : nullptr;
        if (!desc || !instance) {
            return;
        }

        params.resize(desc->param_count);
        for (uint32_t i = 0; i < desc->param_count; ++i) {
            params[i] = desc->params[i].default_value;
        }

        uint32_t float_inputs = 0;
        uint32_t float_outputs = 0;
        uint32_t custom_input_count = 0;
        uint32_t custom_output_count = 0;
        for (uint32_t i = 0; i < desc->port_count; ++i) {
            const auto& port = desc->ports[i];
            if (port.type == VIVID_PORT_FLOAT) {
                if (port.direction == VIVID_PORT_INPUT) {
                    ++float_inputs;
                } else {
                    ++float_outputs;
                }
            }
            if (port.transport == VIVID_PORT_TRANSPORT_CUSTOM_REF ||
                port.transport == VIVID_PORT_TRANSPORT_CUSTOM_VALUE) {
                if (port.direction == VIVID_PORT_INPUT) {
                    ++custom_input_count;
                } else {
                    ++custom_output_count;
                }
            }
        }

        input_floats.assign(float_inputs, 0.0f);
        output_floats.assign(float_outputs, 0.0f);

        input_spreads.resize(desc->port_count);
        output_spreads.resize(desc->port_count);
        input_spread_storage.resize(desc->port_count);
        output_spread_storage.resize(desc->port_count);
        for (uint32_t i = 0; i < desc->port_count; ++i) {
            input_spread_storage[i].assign(spread_capacity, 0.0f);
            output_spread_storage[i].assign(spread_capacity, 0.0f);
            input_spreads[i] = {input_spread_storage[i].data(), 0u, spread_capacity};
            output_spreads[i] = {output_spread_storage[i].data(), 0u, spread_capacity};
        }

        custom_inputs.assign(custom_input_count, nullptr);
        custom_outputs.assign(custom_output_count, nullptr);

        uint32_t file_param_count = 0;
        for (uint32_t i = 0; i < desc->param_count; ++i) {
            if (desc->params[i].type == VIVID_PARAM_FILE ||
                desc->params[i].type == VIVID_PARAM_TEXT) {
                ++file_param_count;
            }
        }
        file_params.resize(file_param_count);
        file_param_ptrs.resize(file_param_count, nullptr);
        uint32_t file_idx = 0;
        for (uint32_t i = 0; i < desc->param_count; ++i) {
            if (desc->params[i].type == VIVID_PARAM_FILE ||
                desc->params[i].type == VIVID_PARAM_TEXT) {
                const char* def = desc->params[i].default_string
                    ? desc->params[i].default_string
                    : "";
                file_params[file_idx] = def;
                file_param_ptrs[file_idx] = file_params[file_idx].c_str();
                ++file_idx;
            }
        }
    }

    ~AudioOpHarness() {
        if (loader && instance) {
            loader->destroy_instance(instance);
        }
    }

    AudioOpHarness(const AudioOpHarness&) = delete;
    AudioOpHarness& operator=(const AudioOpHarness&) = delete;

    int param_index(const char* name) const {
        for (uint32_t i = 0; i < desc->param_count; ++i) {
            if (std::strcmp(desc->params[i].name, name) == 0) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void set_param(const char* name, float value) {
        int idx = param_index(name);
        if (idx >= 0) {
            params[static_cast<size_t>(idx)] = value;
        }
    }

    int string_param_index(const char* name) const {
        uint32_t file_idx = 0;
        for (uint32_t i = 0; i < desc->param_count; ++i) {
            if (desc->params[i].type != VIVID_PARAM_FILE &&
                desc->params[i].type != VIVID_PARAM_TEXT) {
                continue;
            }
            if (std::strcmp(desc->params[i].name, name) == 0) {
                return static_cast<int>(file_idx);
            }
            ++file_idx;
        }
        return -1;
    }

    void set_string_param(const char* name, const std::string& value) {
        int idx = string_param_index(name);
        if (idx >= 0) {
            file_params[static_cast<size_t>(idx)] = value;
            file_param_ptrs[static_cast<size_t>(idx)] =
                file_params[static_cast<size_t>(idx)].c_str();
        }
    }

    void process(uint64_t frame = 0) {
        VividAudioContext ctx{};
        ctx.time = 0.0;
        ctx.delta_time = 0.016;
        ctx.frame = frame;
        ctx.param_values = params.data();
        ctx.input_buffers = nullptr;
        ctx.output_buffers = nullptr;
        ctx.buffer_size = 0;
        ctx.sample_rate = 48000;
        ctx.input_channel_counts = nullptr;
        ctx.output_channel_counts = nullptr;
        ctx.input_spreads = input_spreads.data();
        ctx.output_spreads = output_spreads.data();
        ctx.custom_inputs = custom_inputs.empty() ? nullptr : custom_inputs.data();
        ctx.custom_input_count = static_cast<uint32_t>(custom_inputs.size());
        ctx.input_string_values = nullptr;
        ctx.input_float_values = input_floats.empty() ? nullptr : input_floats.data();
        ctx.output_float_values = output_floats.empty() ? nullptr : output_floats.data();
        ctx.custom_outputs = custom_outputs.empty() ? nullptr : custom_outputs.data();
        ctx.custom_output_count = static_cast<uint32_t>(custom_outputs.size());
        ctx.file_param_values = file_param_ptrs.empty() ? nullptr : file_param_ptrs.data();
        ctx.file_param_count = static_cast<uint32_t>(file_param_ptrs.size());
        ctx.shared_handles = nullptr;
        loader->process_audio(instance, &ctx);
    }
};
