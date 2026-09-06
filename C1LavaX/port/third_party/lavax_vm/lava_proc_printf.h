#pragma once

std::vector<uint8_t> LavaProc::lava_printf(const std::vector<uint8_t> &fmt,
                                           const std::vector<uint32_t> &params)
{
    std::vector<uint8_t> out;

    int iparam = params.size() - 1;
    for (size_t i = 0; i < fmt.size(); i++) {
        // Process the format string
        char cfmt = fmt[i];
        if (cfmt == '\0') {
            out.push_back('\0');
            break;
        } else if (cfmt != '%') {
            // Normal character
            out.push_back(cfmt);
            continue;
        }

        // % format specifier
        i++;

        // Check special formatting
        bool left_adjust = false;   // flag 0x80
        bool zero_fill = false;     // flag 0x40
        for (;;) {
            char c = fmt[i];
            if (c == '-')
                left_adjust = true;
            else if (c == '0')
                zero_fill = true;
            else
                break;
            i++;
        }

        // Check field length
        int len = 0;
        for (;;) {
            char c = fmt[i];
            if (c >= '0' && c <= '9')
                len = len * 10 + c - '0';
            else
                break;
            i++;
        }

        // Check format type
        cfmt = fmt[i];
        if (cfmt == 'd') {
            std::string sval = std::to_string((int32_t)params[iparam--]);
            if (len > sval.length()) {
                char cfill = left_adjust ? ' ' : zero_fill ? '0' : ' ';
                std::string sfill(len - sval.length(), cfill);
                if (left_adjust)
                    sval = sval + sfill;
                else
                    sval = sfill + sval;
            }
            out.insert(out.end(), sval.cbegin(), sval.cend());

        } else if (cfmt == 'f') {
            const uint32_t bits = params[iparam--];
            float value;
            std::memcpy(&value, &bits, sizeof(value));
            std::ostringstream stream;
            stream << value;
            std::string sval = stream.str();
            if (len > static_cast<int>(sval.length())) {
                const std::string fill(len - sval.length(), zero_fill ? '0' : ' ');
                sval = left_adjust ? sval + fill : fill + sval;
            }
            out.insert(out.end(), sval.cbegin(), sval.cend());

        } else if (cfmt == '%') {
            out.push_back(cfmt);

        } else if (cfmt == 'c') {
            char c = params[iparam--];
            out.push_back(c);

        } else if (cfmt == 's') {
            auto const &str = ram.readStringData(params[iparam--]);
            // Remove the 0-terminator
            out.insert(out.end(), str.begin(), str.end() - 1);

        } else {
            // Unknown
            out.push_back(cfmt);
        }
    }

    return out;
}

void LavaProc::lava_op_printf(uint32_t ds0)
{
    const uint32_t nparams = static_cast<uint8_t>(ds0) > 0
        ? static_cast<uint8_t>(ds0) - 1 : 0;
    std::vector<uint32_t> params;
    for (uint32_t i = 0; i < nparams; ++i)
        params.push_back(ram.pop());
    const auto fmt = ram.readStringData(ram.pop());
    consoleWrite(lava_printf(fmt, params));
}

void LavaProc::lava_op_sprintf(uint32_t ds0)
{
    uint32_t nparams = (uint8_t)ds0 - 2;
    std::vector<uint32_t> params;
    for (int i = 0; i < nparams; i++)
        params.push_back(ram.pop());

    auto const &fmt = ram.readStringData(ram.pop());
    uint32_t dst = ram.pop();

    const std::vector<uint8_t> &out = lava_printf(fmt, params);
    ram.writeStringData(dst, out);

    //std::cerr << __func__ << ": " << dst << ", " << (char *)out.data() << std::endl;
}
