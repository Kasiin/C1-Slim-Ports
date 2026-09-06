
void LavaProc::lava_wrap_push_u8(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("push_u8(");
    uint8_t dp0 = data[0];
    DEBUG_WRAP_PRINT((uint32_t)dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_push_u8(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_push_i16(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("push_i16(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_push_i16(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_push_i32(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("push_i32(");
    int32_t dp0 = (int32_t)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_push_i32(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushv_u8(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushv_u8(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushv_u8(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushv_i16(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushv_i16(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushv_i16(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushv_i32(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushv_i32(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushv_i32(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushg_u8(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushg_u8(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushg_u8(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushg_i16(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushg_i16(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushg_i16(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushg_i32(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushg_i32(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushg_i32(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pusha_u8(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pusha_u8(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pusha_u8(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pusha_i16(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pusha_i16(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pusha_i16(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pusha_i32(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pusha_i32(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pusha_i32(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_push_str(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("push_str(");
    const std::vector<uint8_t> &dp0 = data;
    DEBUG_WRAP_PRINT('<' << dp0.size() << '>');
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_push_str(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushlv_u8(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushlv_u8(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushlv_u8(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushlv_i16(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushlv_i16(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushlv_i16(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushlv_i32(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushlv_i32(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushlv_i32(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushlg_char(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushlg_char(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushlg_char(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushlg_int(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushlg_int(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushlg_int(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushlg_long(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushlg_long(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushlg_long(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushla_u8(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushla_u8(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushla_u8(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushla_i32(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushla_i32(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushla_i32(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pushl_i32(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pushl_i32(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pushl_i32(dp0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_neg(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("neg(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_neg(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pre_inc(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pre_inc(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pre_inc(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pre_dec(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pre_dec(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_pre_dec(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_post_inc(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("post_inc(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_post_inc(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_post_dec(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("post_dec(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_post_dec(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_add(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("add(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_add(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_sub(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("sub(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_sub(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_and(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("and(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_and(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_or(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("or(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_or(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_xor(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("xor(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_xor(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_land(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("land(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_land(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_lor(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("lor(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_lor(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_lnot(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("lnot(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_lnot(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_mul(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("mul(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_mul(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_div(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("div(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_div(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_mod(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("mod(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_mod(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_lshift(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("lshift(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_lshift(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_rshift(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("rshift(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_rshift(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_equ(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("equ(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_equ(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_neq(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("neq(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_neq(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_le(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("le(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_le(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_ge(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("ge(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_ge(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_great(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("great(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_great(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_less(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("less(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_less(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_let(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("let(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_let(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_ptr(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("ptr(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_ptr(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_cptr(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("cptr(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_cptr(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_pop(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pop(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_pop(ds0);
}

void LavaProc::lava_wrap_jmpe(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("jmpe(");
    uint32_t dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_jmpe(dp0);
}

void LavaProc::lava_wrap_jmpn(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("jmpn(");
    uint32_t dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_jmpn(dp0);
}

void LavaProc::lava_wrap_jmp(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("jmp(");
    uint32_t dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_jmp(dp0);
}

void LavaProc::lava_wrap_set_sp(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("set_sp(");
    uint32_t dp0;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_set_sp(dp0);
}

void LavaProc::lava_wrap_call(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("call(");
    uint32_t dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_call(dp0);
}

void LavaProc::lava_wrap_push_frame(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("push_frame(");
    uint32_t dp0;
    uint8_t dp1;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
        dp1 = data[2];
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
        dp1 = data[3];
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(", " << (uint32_t)dp1);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_push_frame(dp0, dp1);
}

void LavaProc::lava_wrap_pop_frame(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("pop_frame(");
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_pop_frame();
}

void LavaProc::lava_wrap_quit(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("quit(");
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_quit();
}

void LavaProc::lava_wrap_preset(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("preset(");
    uint32_t dp0;
    uint16_t dp1;
    std::vector<uint8_t> dp2;
    if (rambits <= 16) {
        dp0 = (uint32_t)(data[0] | (data[1] << 8));
        dp1 = data[2] | (data[3] << 8);
        dp2 = std::vector<uint8_t>(data.cbegin() + 4, data.cend());
    } else {
        dp0 = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
        dp1 = data[3] | (data[4] << 8);
        dp2 = std::vector<uint8_t>(data.cbegin() + 5, data.cend());
    }
    DEBUG_WRAP_PRINT(dp0);
    DEBUG_WRAP_PRINT(", " << dp1);
    DEBUG_WRAP_PRINT(", <" << dp2.size() << '>');
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_preset(dp0, dp2);
}

void LavaProc::lava_wrap_qadd(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qadd(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qadd(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qsub(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qsub(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qsub(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qmul(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qmul(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qmul(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qdiv(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qdiv(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qdiv(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qmod(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qmod(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qmod(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qlshift(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qlshift(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qlshift(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qrshift(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qrshift(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qrshift(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qequ(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qequ(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qequ(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qneq(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qneq(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qneq(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qgreat(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qgreat(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qgreat(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qless(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qless(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qless(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qge(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qge(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qge(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_qle(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("qle(");
    int16_t dp0 = (int16_t)(data[0] | (data[1] << 8));
    DEBUG_WRAP_PRINT(dp0);
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_qle(dp0, ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_getchar(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("getchar(");
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_getchar();
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_printf(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("printf(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_printf(ds0);
}

void LavaProc::lava_wrap_strcpy(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("strcpy(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_strcpy(ds0, ds1);
}

void LavaProc::lava_wrap_strlen(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("strlen(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_strlen(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_setscreen(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("setscreen(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_setscreen(ds0);
}

void LavaProc::lava_wrap_delay(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("delay(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_delay(ds0);
}

void LavaProc::lava_wrap_writeblock(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("writeblock(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    uint32_t ds3 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds3);
    uint32_t ds4 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds4);
    uint32_t ds5 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds5);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_writeblock(ds0, ds1, ds2, ds3, ds4, ds5);
}

void LavaProc::lava_wrap_fbflush(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("fbflush(");
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_fbflush();
}

void LavaProc::lava_wrap_textout(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("textout(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    uint32_t ds3 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds3);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_textout(ds0, ds1, ds2, ds3);
}

void LavaProc::lava_wrap_block(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("block(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    uint32_t ds3 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds3);
    uint32_t ds4 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds4);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_block(ds0, ds1, ds2, ds3, ds4);
}

void LavaProc::lava_wrap_rectangle(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("rectangle(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    uint32_t ds3 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds3);
    uint32_t ds4 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds4);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_rectangle(ds0, ds1, ds2, ds3, ds4);
}

void LavaProc::lava_wrap_exit(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("exit(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_exit(ds0);
}

void LavaProc::lava_wrap_clearscreen(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("clearscreen(");
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_clearscreen();
}

void LavaProc::lava_wrap_rand(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("rand(");
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_rand();
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_srand(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("srand(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_srand(ds0);
}

void LavaProc::lava_wrap_locate(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("locate(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_locate(ds0, ds1);
}

void LavaProc::lava_wrap_inkey(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("inkey(");
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_inkey();
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_point(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("point(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_point(ds0, ds1, ds2);
}

void LavaProc::lava_wrap_line(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("line(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    uint32_t ds3 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds3);
    uint32_t ds4 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds4);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_line(ds0, ds1, ds2, ds3, ds4);
}

void LavaProc::lava_wrap_circle(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("circle(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    uint32_t ds3 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds3);
    uint32_t ds4 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds4);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_circle(ds0, ds1, ds2, ds3, ds4);
}

void LavaProc::lava_wrap_isalpha(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("isalpha(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_isalpha(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_strcat(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("strcat(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_strcat(ds0, ds1);
}

void LavaProc::lava_wrap_strchr(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("strchr(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_strchr(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_strcmp(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("strcmp(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_strcmp(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_strstr(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("strstr(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_strstr(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_memset(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("memset(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_memset(ds0, ds1, ds2);
}

void LavaProc::lava_wrap_memcpy(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("memcpy(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_memcpy(ds0, ds1, ds2);
}

void LavaProc::lava_wrap_fopen(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("fopen(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_fopen(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_fclose(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("fclose(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_fclose(ds0);
}

void LavaProc::lava_wrap_fread(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("fread(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    uint32_t ds3 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds3);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_fread(ds0, ds1, ds2, ds3);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_fwrite(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("fwrite(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    uint32_t ds3 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds3);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_fwrite(ds0, ds1, ds2, ds3);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_fseek(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("fseek(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_fseek(ds0, ds1, ds2);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_rewind(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("rewind(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_rewind(ds0);
}

void LavaProc::lava_wrap_sprintf(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("sprintf(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_sprintf(ds0);
}

void LavaProc::lava_wrap_deletefile(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("deletefile(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_deletefile(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_gettick(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("gettick(");
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_gettick();
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_checkkey(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("checkkey(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_checkkey(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_crc16(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("crc16(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_crc16(ds0, ds1);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_encrypt(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("encrypt(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_encrypt(ds0, ds1, ds2);
}

void LavaProc::lava_wrap_gettime(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("gettime(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_gettime(ds0);
}

void LavaProc::lava_wrap_xdraw(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("xdraw(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_xdraw(ds0);
}

void LavaProc::lava_wrap_releasekey(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("releasekey(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_releasekey(ds0);
}

void LavaProc::lava_wrap_getblock(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("getblock(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    uint32_t ds1 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds1);
    uint32_t ds2 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds2);
    uint32_t ds3 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds3);
    uint32_t ds4 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds4);
    uint32_t ds5 = ram.pop();
    DEBUG_WRAP_PRINT(", ");
    DEBUG_WRAP_PRINT(ds5);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_getblock(ds0, ds1, ds2, ds3, ds4, ds5);
}

void LavaProc::lava_wrap_setgraphmode(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("setgraphmode(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    pc += 1 + data.size();
    uint32_t ret = lava_op_setgraphmode(ds0);
    ram.push(ret);
    DEBUG_WRAP_PRINT(" -> " << ret << std::endl);
}

void LavaProc::lava_wrap_setbgcolor(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("setbgcolor(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_setbgcolor(ds0);
}

void LavaProc::lava_wrap_setfgcolor(const std::vector<uint8_t> &data)
{
    DEBUG_WRAP_PRINT("setfgcolor(");
    uint32_t ds0 = ram.pop();
    DEBUG_WRAP_PRINT(ds0);
    DEBUG_WRAP_PRINT(")");
    DEBUG_WRAP_PRINT(std::endl);
    pc += 1 + data.size();
    lava_op_setfgcolor(ds0);
}

std::unordered_map<uint8_t, LavaProc::op_info_t> LavaProc::op_info = {
    {0x01, op_info_t{"push_u8", OpParam1, &LavaProc::lava_wrap_push_u8}},
    {0x02, op_info_t{"push_i16", OpParam2, &LavaProc::lava_wrap_push_i16}},
    {0x03, op_info_t{"push_i32", OpParam4, &LavaProc::lava_wrap_push_i32}},
    {0x04, op_info_t{"pushv_u8", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushv_u8}},
    {0x05, op_info_t{"pushv_i16", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushv_i16}},
    {0x06, op_info_t{"pushv_i32", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushv_i32}},
    {0x07, op_info_t{"pushg_u8", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushg_u8}},
    {0x08, op_info_t{"pushg_i16", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushg_i16}},
    {0x09, op_info_t{"pushg_i32", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushg_i32}},
    {0x0a, op_info_t{"pusha_u8", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pusha_u8}},
    {0x0b, op_info_t{"pusha_i16", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pusha_i16}},
    {0x0c, op_info_t{"pusha_i32", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pusha_i32}},
    {0x0d, op_info_t{"push_str", (op_len_t)(OpParam0 | OpParamStr), &LavaProc::lava_wrap_push_str}},
    {0x0e, op_info_t{"pushlv_u8", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushlv_u8}},
    {0x0f, op_info_t{"pushlv_i16", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushlv_i16}},
    {0x10, op_info_t{"pushlv_i32", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushlv_i32}},
    {0x11, op_info_t{"pushlg_char", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushlg_char}},
    {0x12, op_info_t{"pushlg_int", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushlg_int}},
    {0x13, op_info_t{"pushlg_long", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushlg_long}},
    {0x14, op_info_t{"pushla_u8", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushla_u8}},
    {0x15, op_info_t{"pushla_i16", (op_len_t)(OpParam0 | OpParamAddr), {}}},
    {0x16, op_info_t{"pushla_i32", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushla_i32}},
    {0x17, op_info_t{"pusha_i32", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pusha_i32}},
    {0x18, op_info_t{"pushla_i32", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushla_i32}},
    {0x19, op_info_t{"pushl_i32", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_pushl_i32}},
    {0x1a, op_info_t{"push_text", OpParam0, {}}},
    {0x1b, op_info_t{"push_graph", OpParam0, {}}},
    {0x1c, op_info_t{"neg", OpParam0, &LavaProc::lava_wrap_neg}},
    {0x1d, op_info_t{"pre_inc", OpParam0, &LavaProc::lava_wrap_pre_inc}},
    {0x1e, op_info_t{"pre_dec", OpParam0, &LavaProc::lava_wrap_pre_dec}},
    {0x1f, op_info_t{"post_inc", OpParam0, &LavaProc::lava_wrap_post_inc}},
    {0x20, op_info_t{"post_dec", OpParam0, &LavaProc::lava_wrap_post_dec}},
    {0x21, op_info_t{"add", OpParam0, &LavaProc::lava_wrap_add}},
    {0x22, op_info_t{"sub", OpParam0, &LavaProc::lava_wrap_sub}},
    {0x23, op_info_t{"and", OpParam0, &LavaProc::lava_wrap_and}},
    {0x24, op_info_t{"or", OpParam0, &LavaProc::lava_wrap_or}},
    {0x25, op_info_t{"not", OpParam0, {}}},
    {0x26, op_info_t{"xor", OpParam0, &LavaProc::lava_wrap_xor}},
    {0x27, op_info_t{"land", OpParam0, &LavaProc::lava_wrap_land}},
    {0x28, op_info_t{"lor", OpParam0, &LavaProc::lava_wrap_lor}},
    {0x29, op_info_t{"lnot", OpParam0, &LavaProc::lava_wrap_lnot}},
    {0x2a, op_info_t{"mul", OpParam0, &LavaProc::lava_wrap_mul}},
    {0x2b, op_info_t{"div", OpParam0, &LavaProc::lava_wrap_div}},
    {0x2c, op_info_t{"mod", OpParam0, &LavaProc::lava_wrap_mod}},
    {0x2d, op_info_t{"lshift", OpParam0, &LavaProc::lava_wrap_lshift}},
    {0x2e, op_info_t{"rshift", OpParam0, &LavaProc::lava_wrap_rshift}},
    {0x2f, op_info_t{"equ", OpParam0, &LavaProc::lava_wrap_equ}},
    {0x30, op_info_t{"neq", OpParam0, &LavaProc::lava_wrap_neq}},
    {0x31, op_info_t{"le", OpParam0, &LavaProc::lava_wrap_le}},
    {0x32, op_info_t{"ge", OpParam0, &LavaProc::lava_wrap_ge}},
    {0x33, op_info_t{"great", OpParam0, &LavaProc::lava_wrap_great}},
    {0x34, op_info_t{"less", OpParam0, &LavaProc::lava_wrap_less}},
    {0x35, op_info_t{"let", OpParam0, &LavaProc::lava_wrap_let}},
    {0x36, op_info_t{"ptr", OpParam0, &LavaProc::lava_wrap_ptr}},
    {0x37, op_info_t{"cptr", OpParam0, &LavaProc::lava_wrap_cptr}},
    {0x38, op_info_t{"pop", OpParam0, &LavaProc::lava_wrap_pop}},
    {0x39, op_info_t{"jmpe", OpParam3, &LavaProc::lava_wrap_jmpe}},
    {0x3a, op_info_t{"jmpn", OpParam3, &LavaProc::lava_wrap_jmpn}},
    {0x3b, op_info_t{"jmp", OpParam3, &LavaProc::lava_wrap_jmp}},
    {0x3c, op_info_t{"set_sp", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_set_sp}},
    {0x3d, op_info_t{"call", OpParam3, &LavaProc::lava_wrap_call}},
    {0x3e, op_info_t{"push_frame", (op_len_t)(OpParam1 | OpParamAddr), &LavaProc::lava_wrap_push_frame}},
    {0x3f, op_info_t{"pop_frame", OpParam0, &LavaProc::lava_wrap_pop_frame}},
    {0x40, op_info_t{"quit", OpParam0, &LavaProc::lava_wrap_quit}},
    {0x41, op_info_t{"preset", (op_len_t)(OpParam2 | OpParamAddr | OpParamPreset), &LavaProc::lava_wrap_preset}},
    {0x42, op_info_t{"push_gbuf", OpParam0, {}}},
    {0x43, op_info_t{"secret", OpParam1, {}}},
    {0x44, op_info_t{"loadall", OpParam0, {}}},
    {0x45, op_info_t{"qadd", OpParam2, &LavaProc::lava_wrap_qadd}},
    {0x46, op_info_t{"qsub", OpParam2, &LavaProc::lava_wrap_qsub}},
    {0x47, op_info_t{"qmul", OpParam2, &LavaProc::lava_wrap_qmul}},
    {0x48, op_info_t{"qdiv", OpParam2, &LavaProc::lava_wrap_qdiv}},
    {0x49, op_info_t{"qmod", OpParam2, &LavaProc::lava_wrap_qmod}},
    {0x4a, op_info_t{"qlshift", OpParam2, &LavaProc::lava_wrap_qlshift}},
    {0x4b, op_info_t{"qrshift", OpParam2, &LavaProc::lava_wrap_qrshift}},
    {0x4c, op_info_t{"qequ", OpParam2, &LavaProc::lava_wrap_qequ}},
    {0x4d, op_info_t{"qneq", OpParam2, &LavaProc::lava_wrap_qneq}},
    {0x4e, op_info_t{"qgreat", OpParam2, &LavaProc::lava_wrap_qgreat}},
    {0x4f, op_info_t{"qless", OpParam2, &LavaProc::lava_wrap_qless}},
    {0x50, op_info_t{"qge", OpParam2, &LavaProc::lava_wrap_qge}},
    {0x51, op_info_t{"qle", OpParam2, &LavaProc::lava_wrap_qle}},
    {0x52, op_info_t{"iptr", OpParam0, {}}},
    {0x53, op_info_t{"lptr", OpParam0, {}}},
    {0x54, op_info_t{"int_to_float", OpParam0, {}}},
    {0x55, op_info_t{"float_to_int", OpParam0, {}}},
    {0x56, op_info_t{"float_add", OpParam0, {}}},
    {0x57, op_info_t{"float_add_i", OpParam0, {}}},
    {0x58, op_info_t{"int_add_float", OpParam0, {}}},
    {0x59, op_info_t{"float_sub", OpParam0, {}}},
    {0x5a, op_info_t{"float_sub_i", OpParam0, {}}},
    {0x5b, op_info_t{"int_sub_float", OpParam0, {}}},
    {0x5c, op_info_t{"float_mul", OpParam0, {}}},
    {0x5d, op_info_t{"float_mul_i", OpParam0, {}}},
    {0x5e, op_info_t{"int_mul_float", OpParam0, {}}},
    {0x5f, op_info_t{"float_div", OpParam0, {}}},
    {0x60, op_info_t{"float_div_i", OpParam0, {}}},
    {0x61, op_info_t{"int_div_float", OpParam0, {}}},
    {0x62, op_info_t{"float_neg", OpParam0, {}}},
    {0x63, op_info_t{"float_less", OpParam0, {}}},
    {0x64, op_info_t{"float_great", OpParam0, {}}},
    {0x65, op_info_t{"float_equ", OpParam0, {}}},
    {0x66, op_info_t{"float_neq", OpParam0, {}}},
    {0x67, op_info_t{"float_le", OpParam0, {}}},
    {0x68, op_info_t{"float_ge", OpParam0, {}}},
    {0x69, op_info_t{"float_abs", OpParam0, {}}},
    {0x6a, op_info_t{"ciptr", OpParam0, {}}},
    {0x6b, op_info_t{"clptr", OpParam0, {}}},
    {0x6c, op_info_t{"long_to_char", OpParam0, {}}},
    {0x6d, op_info_t{"long_to_int", OpParam0, {}}},
    {0x6e, op_info_t{"letx", OpParam1, &LavaProc::lava_wrap_letx}},
    {0x6f, op_info_t{"push_ax", (op_len_t)(OpParam0 | OpParamAddr), &LavaProc::lava_wrap_push_ax}},
    {0x70, op_info_t{"idx", OpParam1, &LavaProc::lava_wrap_idx}},
    {0x71, op_info_t{"pass", OpParam1, &LavaProc::lava_wrap_pass}},
    {0x72, op_info_t{"void", OpParam0, &LavaProc::lava_wrap_void}},
    {0x73, op_info_t{"debug", OpParam3, {}}},
    {0x74, op_info_t{"funcid", OpParam3, {}}},
    {0x80, op_info_t{"putchar", OpParam0, &LavaProc::lava_wrap_putchar}},
    {0x81, op_info_t{"getchar", OpParam0, &LavaProc::lava_wrap_getchar}},
    {0x82, op_info_t{"printf", OpParam0, &LavaProc::lava_wrap_printf}},
    {0x83, op_info_t{"strcpy", OpParam0, &LavaProc::lava_wrap_strcpy}},
    {0x84, op_info_t{"strlen", OpParam0, &LavaProc::lava_wrap_strlen}},
    {0x85, op_info_t{"setscreen", OpParam0, &LavaProc::lava_wrap_setscreen}},
    {0x86, op_info_t{"updatelcd", OpParam0, {}}},
    {0x87, op_info_t{"delay", OpParam0, &LavaProc::lava_wrap_delay}},
    {0x88, op_info_t{"writeblock", OpParam0, &LavaProc::lava_wrap_writeblock}},
    {0x89, op_info_t{"fbflush", OpParam0, &LavaProc::lava_wrap_fbflush}},
    {0x8a, op_info_t{"textout", OpParam0, &LavaProc::lava_wrap_textout}},
    {0x8b, op_info_t{"block", OpParam0, &LavaProc::lava_wrap_block}},
    {0x8c, op_info_t{"rectangle", OpParam0, &LavaProc::lava_wrap_rectangle}},
    {0x8d, op_info_t{"exit", OpParam0, &LavaProc::lava_wrap_exit}},
    {0x8e, op_info_t{"clearscreen", OpParam0, &LavaProc::lava_wrap_clearscreen}},
    {0x8f, op_info_t{"abs", OpParam0, {}}},
    {0x90, op_info_t{"rand", OpParam0, &LavaProc::lava_wrap_rand}},
    {0x91, op_info_t{"srand", OpParam0, &LavaProc::lava_wrap_srand}},
    {0x92, op_info_t{"locate", OpParam0, &LavaProc::lava_wrap_locate}},
    {0x93, op_info_t{"inkey", OpParam0, &LavaProc::lava_wrap_inkey}},
    {0x94, op_info_t{"point", OpParam0, &LavaProc::lava_wrap_point}},
    {0x95, op_info_t{"getpoint", OpParam0, {}}},
    {0x96, op_info_t{"line", OpParam0, &LavaProc::lava_wrap_line}},
    {0x97, op_info_t{"box", OpParam0, {}}},
    {0x98, op_info_t{"circle", OpParam0, &LavaProc::lava_wrap_circle}},
    {0x99, op_info_t{"ellipse", OpParam0, {}}},
    {0x9a, op_info_t{"beep", OpParam0, {}}},
    {0x9b, op_info_t{"isalnum", OpParam0, {}}},
    {0x9c, op_info_t{"isalpha", OpParam0, &LavaProc::lava_wrap_isalpha}},
    {0x9d, op_info_t{"iscntrl", OpParam0, {}}},
    {0x9e, op_info_t{"isdigit", OpParam0, {}}},
    {0x9f, op_info_t{"isgraph", OpParam0, {}}},
    {0xa0, op_info_t{"islower", OpParam0, {}}},
    {0xa1, op_info_t{"isprint", OpParam0, {}}},
    {0xa2, op_info_t{"ispunct", OpParam0, {}}},
    {0xa3, op_info_t{"isspace", OpParam0, {}}},
    {0xa4, op_info_t{"isupper", OpParam0, {}}},
    {0xa5, op_info_t{"isxdigit", OpParam0, {}}},
    {0xa6, op_info_t{"strcat", OpParam0, &LavaProc::lava_wrap_strcat}},
    {0xa7, op_info_t{"strchr", OpParam0, &LavaProc::lava_wrap_strchr}},
    {0xa8, op_info_t{"strcmp", OpParam0, &LavaProc::lava_wrap_strcmp}},
    {0xa9, op_info_t{"strstr", OpParam0, &LavaProc::lava_wrap_strstr}},
    {0xaa, op_info_t{"tolower", OpParam0, &LavaProc::lava_wrap_tolower}},
    {0xab, op_info_t{"toupper", OpParam0, &LavaProc::lava_wrap_toupper}},
    {0xac, op_info_t{"memset", OpParam0, &LavaProc::lava_wrap_memset}},
    {0xad, op_info_t{"memcpy", OpParam0, &LavaProc::lava_wrap_memcpy}},
    {0xae, op_info_t{"fopen", OpParam0, &LavaProc::lava_wrap_fopen}},
    {0xaf, op_info_t{"fclose", OpParam0, &LavaProc::lava_wrap_fclose}},
    {0xb0, op_info_t{"fread", OpParam0, &LavaProc::lava_wrap_fread}},
    {0xb1, op_info_t{"fwrite", OpParam0, &LavaProc::lava_wrap_fwrite}},
    {0xb2, op_info_t{"fseek", OpParam0, &LavaProc::lava_wrap_fseek}},
    {0xb3, op_info_t{"ftell", OpParam0, {}}},
    {0xb4, op_info_t{"feof", OpParam0, {}}},
    {0xb5, op_info_t{"rewind", OpParam0, &LavaProc::lava_wrap_rewind}},
    {0xb6, op_info_t{"getc", OpParam0, {}}},
    {0xb7, op_info_t{"putc", OpParam0, {}}},
    {0xb8, op_info_t{"sprintf", OpParam0, &LavaProc::lava_wrap_sprintf}},
    {0xb9, op_info_t{"makedir", OpParam0, &LavaProc::lava_wrap_makedir}},
    {0xba, op_info_t{"deletefile", OpParam0, &LavaProc::lava_wrap_deletefile}},
    {0xbb, op_info_t{"gettick", OpParam0, &LavaProc::lava_wrap_gettick}},
    {0xbc, op_info_t{"checkkey", OpParam0, &LavaProc::lava_wrap_checkkey}},
    {0xbd, op_info_t{"memmove", OpParam0, {}}},
    {0xbe, op_info_t{"crc16", OpParam0, &LavaProc::lava_wrap_crc16}},
    {0xbf, op_info_t{"encrypt", OpParam0, &LavaProc::lava_wrap_encrypt}},
    {0xc0, op_info_t{"chdir", OpParam0, &LavaProc::lava_wrap_chdir}},
    {0xc1, op_info_t{"filelist", OpParam0, {}}},
    {0xc2, op_info_t{"gettime", OpParam0, &LavaProc::lava_wrap_gettime}},
    {0xc3, op_info_t{"settime", OpParam0, {}}},
    {0xc4, op_info_t{"getword", OpParam0, {}}},
    {0xc5, op_info_t{"xdraw", OpParam0, &LavaProc::lava_wrap_xdraw}},
    {0xc6, op_info_t{"releasekey", OpParam0, &LavaProc::lava_wrap_releasekey}},
    {0xc7, op_info_t{"getblock", OpParam0, &LavaProc::lava_wrap_getblock}},
    {0xc8, op_info_t{"sin", OpParam0, {}}},
    {0xc9, op_info_t{"cos", OpParam0, {}}},
    {0xca, op_info_t{"fill", OpParam0, {}}},
    {0xcb, op_info_t{"setgraphmode", OpParam0, &LavaProc::lava_wrap_setgraphmode}},
    {0xcc, op_info_t{"setbgcolor", OpParam0, &LavaProc::lava_wrap_setbgcolor}},
    {0xcd, op_info_t{"setfgcolor", OpParam0, &LavaProc::lava_wrap_setfgcolor}},
    {0xce, op_info_t{"setlist", OpParam0, {}}},
    {0xcf, op_info_t{"fade", OpParam0, {}}},
    {0xd0, op_info_t{"exec", OpParam0, &LavaProc::lava_wrap_exec}},
    {0xd1, op_info_t{"findfile", OpParam0, &LavaProc::lava_wrap_findfile}},
    {0xd2, op_info_t{"getfilenum", OpParam0, &LavaProc::lava_wrap_getfilenum}},
    {0xd3, op_info_t{"system", OpParam0, {}}},
    {0xd4, op_info_t{"math", OpParam0, {}}},
    {0xd5, op_info_t{"setpalette", OpParam0, &LavaProc::lava_wrap_setpalette}},
    {0xd6, op_info_t{"getcmdline", OpParam0, {}}},
};
