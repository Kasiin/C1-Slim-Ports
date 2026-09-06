
void lava_wrap_push_u8(const std::vector<uint8_t> &data);
inline uint32_t lava_op_push_u8(uint8_t dp0);

void lava_wrap_push_i16(const std::vector<uint8_t> &data);
inline uint32_t lava_op_push_i16(int16_t dp0);

void lava_wrap_push_i32(const std::vector<uint8_t> &data);
inline uint32_t lava_op_push_i32(int32_t dp0);

void lava_wrap_pushv_u8(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushv_u8(uint32_t dp0);

void lava_wrap_pushv_i16(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushv_i16(uint32_t dp0);

void lava_wrap_pushv_i32(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushv_i32(uint32_t dp0);

void lava_wrap_pushg_u8(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushg_u8(uint32_t dp0, uint32_t ds0);

void lava_wrap_pushg_i16(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushg_i16(uint32_t dp0, uint32_t ds0);

void lava_wrap_pushg_i32(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushg_i32(uint32_t dp0, uint32_t ds0);

void lava_wrap_pusha_u8(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pusha_u8(uint32_t dp0, uint32_t ds0);

void lava_wrap_pusha_i16(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pusha_i16(uint32_t dp0, uint32_t ds0);

void lava_wrap_pusha_i32(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pusha_i32(uint32_t dp0, uint32_t ds0);

void lava_wrap_push_str(const std::vector<uint8_t> &data);
inline uint32_t lava_op_push_str(const std::vector<uint8_t> &dp0);

void lava_wrap_pushlv_u8(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushlv_u8(uint32_t dp0);

void lava_wrap_pushlv_i16(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushlv_i16(uint32_t dp0);

void lava_wrap_pushlv_i32(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushlv_i32(uint32_t dp0);

void lava_wrap_pushlg_char(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushlg_char(uint32_t dp0, uint32_t ds0);

void lava_wrap_pushlg_int(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushlg_int(uint32_t dp0, uint32_t ds0);

void lava_wrap_pushlg_long(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushlg_long(uint32_t dp0, uint32_t ds0);

void lava_wrap_pushla_u8(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushla_u8(uint32_t dp0, uint32_t ds0);

void lava_wrap_pushla_i32(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushla_i32(uint32_t dp0, uint32_t ds0);

void lava_wrap_pushl_i32(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pushl_i32(uint32_t dp0);

void lava_wrap_neg(const std::vector<uint8_t> &data);
inline uint32_t lava_op_neg(uint32_t ds0);

void lava_wrap_pre_inc(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pre_inc(uint32_t ds0);

void lava_wrap_pre_dec(const std::vector<uint8_t> &data);
inline uint32_t lava_op_pre_dec(uint32_t ds0);

void lava_wrap_post_inc(const std::vector<uint8_t> &data);
inline uint32_t lava_op_post_inc(uint32_t ds0);

void lava_wrap_post_dec(const std::vector<uint8_t> &data);
inline uint32_t lava_op_post_dec(uint32_t ds0);

void lava_wrap_add(const std::vector<uint8_t> &data);
inline uint32_t lava_op_add(uint32_t ds0, uint32_t ds1);

void lava_wrap_sub(const std::vector<uint8_t> &data);
inline uint32_t lava_op_sub(uint32_t ds0, uint32_t ds1);

void lava_wrap_and(const std::vector<uint8_t> &data);
inline uint32_t lava_op_and(uint32_t ds0, uint32_t ds1);

void lava_wrap_or(const std::vector<uint8_t> &data);
inline uint32_t lava_op_or(uint32_t ds0, uint32_t ds1);

void lava_wrap_xor(const std::vector<uint8_t> &data);
inline uint32_t lava_op_xor(uint32_t ds0, uint32_t ds1);

void lava_wrap_land(const std::vector<uint8_t> &data);
inline uint32_t lava_op_land(uint32_t ds0, uint32_t ds1);

void lava_wrap_lor(const std::vector<uint8_t> &data);
inline uint32_t lava_op_lor(uint32_t ds0, uint32_t ds1);

void lava_wrap_lnot(const std::vector<uint8_t> &data);
inline uint32_t lava_op_lnot(uint32_t ds0);

void lava_wrap_mul(const std::vector<uint8_t> &data);
inline uint32_t lava_op_mul(uint32_t ds0, uint32_t ds1);

void lava_wrap_div(const std::vector<uint8_t> &data);
inline uint32_t lava_op_div(uint32_t ds0, uint32_t ds1);

void lava_wrap_mod(const std::vector<uint8_t> &data);
inline uint32_t lava_op_mod(uint32_t ds0, uint32_t ds1);

void lava_wrap_lshift(const std::vector<uint8_t> &data);
inline uint32_t lava_op_lshift(uint32_t ds0, uint32_t ds1);

void lava_wrap_rshift(const std::vector<uint8_t> &data);
inline uint32_t lava_op_rshift(uint32_t ds0, uint32_t ds1);

void lava_wrap_equ(const std::vector<uint8_t> &data);
inline uint32_t lava_op_equ(uint32_t ds0, uint32_t ds1);

void lava_wrap_neq(const std::vector<uint8_t> &data);
inline uint32_t lava_op_neq(uint32_t ds0, uint32_t ds1);

void lava_wrap_le(const std::vector<uint8_t> &data);
inline uint32_t lava_op_le(uint32_t ds0, uint32_t ds1);

void lava_wrap_ge(const std::vector<uint8_t> &data);
inline uint32_t lava_op_ge(uint32_t ds0, uint32_t ds1);

void lava_wrap_great(const std::vector<uint8_t> &data);
inline uint32_t lava_op_great(uint32_t ds0, uint32_t ds1);

void lava_wrap_less(const std::vector<uint8_t> &data);
inline uint32_t lava_op_less(uint32_t ds0, uint32_t ds1);

void lava_wrap_let(const std::vector<uint8_t> &data);
inline uint32_t lava_op_let(uint32_t ds0, uint32_t ds1);

void lava_wrap_ptr(const std::vector<uint8_t> &data);
inline uint32_t lava_op_ptr(uint32_t ds0);

void lava_wrap_cptr(const std::vector<uint8_t> &data);
inline uint32_t lava_op_cptr(uint32_t ds0);

void lava_wrap_pop(const std::vector<uint8_t> &data);
inline void lava_op_pop(uint32_t ds0);

void lava_wrap_jmpe(const std::vector<uint8_t> &data);
inline void lava_op_jmpe(uint32_t dp0);

void lava_wrap_jmpn(const std::vector<uint8_t> &data);
inline void lava_op_jmpn(uint32_t dp0);

void lava_wrap_jmp(const std::vector<uint8_t> &data);
inline void lava_op_jmp(uint32_t dp0);

void lava_wrap_set_sp(const std::vector<uint8_t> &data);
inline void lava_op_set_sp(uint32_t dp0);

void lava_wrap_call(const std::vector<uint8_t> &data);
inline void lava_op_call(uint32_t dp0);

void lava_wrap_push_frame(const std::vector<uint8_t> &data);
inline void lava_op_push_frame(uint32_t dp0, uint8_t dp1);

void lava_wrap_pop_frame(const std::vector<uint8_t> &data);
inline void lava_op_pop_frame();

void lava_wrap_quit(const std::vector<uint8_t> &data);
inline void lava_op_quit();

void lava_wrap_preset(const std::vector<uint8_t> &data);
inline void lava_op_preset(uint32_t dp0, const std::vector<uint8_t> &dp2);

void lava_wrap_qadd(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qadd(int16_t dp0, uint32_t ds0);

void lava_wrap_qsub(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qsub(int16_t dp0, uint32_t ds0);

void lava_wrap_qmul(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qmul(int16_t dp0, uint32_t ds0);

void lava_wrap_qdiv(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qdiv(int16_t dp0, uint32_t ds0);

void lava_wrap_qmod(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qmod(int16_t dp0, uint32_t ds0);

void lava_wrap_qlshift(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qlshift(int16_t dp0, uint32_t ds0);

void lava_wrap_qrshift(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qrshift(int16_t dp0, uint32_t ds0);

void lava_wrap_qequ(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qequ(int16_t dp0, uint32_t ds0);

void lava_wrap_qneq(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qneq(int16_t dp0, uint32_t ds0);

void lava_wrap_qgreat(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qgreat(int16_t dp0, uint32_t ds0);

void lava_wrap_qless(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qless(int16_t dp0, uint32_t ds0);

void lava_wrap_qge(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qge(int16_t dp0, uint32_t ds0);

void lava_wrap_qle(const std::vector<uint8_t> &data);
inline uint32_t lava_op_qle(int16_t dp0, uint32_t ds0);

void lava_wrap_pass(const std::vector<uint8_t> &data);
void lava_wrap_void(const std::vector<uint8_t> &data);
void lava_wrap_letx(const std::vector<uint8_t> &data);
void lava_wrap_idx(const std::vector<uint8_t> &data);
void lava_wrap_push_ax(const std::vector<uint8_t> &data);
void lava_wrap_tolower(const std::vector<uint8_t> &data);
void lava_wrap_toupper(const std::vector<uint8_t> &data);
void lava_wrap_putchar(const std::vector<uint8_t> &data);
void lava_wrap_makedir(const std::vector<uint8_t> &data);
void lava_wrap_findfile(const std::vector<uint8_t> &data);
void lava_wrap_getfilenum(const std::vector<uint8_t> &data);
void lava_wrap_chdir(const std::vector<uint8_t> &data);
void lava_wrap_exec(const std::vector<uint8_t> &data);
void lava_wrap_setpalette(const std::vector<uint8_t> &data);

void lava_wrap_getchar(const std::vector<uint8_t> &data);
inline uint32_t lava_op_getchar();

void lava_wrap_printf(const std::vector<uint8_t> &data);
inline void lava_op_printf(uint32_t ds0);

void lava_wrap_strcpy(const std::vector<uint8_t> &data);
inline void lava_op_strcpy(uint32_t ds0, uint32_t ds1);

void lava_wrap_strlen(const std::vector<uint8_t> &data);
inline uint32_t lava_op_strlen(uint32_t ds0);

void lava_wrap_setscreen(const std::vector<uint8_t> &data);
inline void lava_op_setscreen(uint32_t ds0);

void lava_wrap_delay(const std::vector<uint8_t> &data);
inline void lava_op_delay(uint32_t ds0);

void lava_wrap_writeblock(const std::vector<uint8_t> &data);
inline void lava_op_writeblock(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4, uint32_t ds5);

void lava_wrap_fbflush(const std::vector<uint8_t> &data);
inline void lava_op_fbflush();

void lava_wrap_textout(const std::vector<uint8_t> &data);
inline void lava_op_textout(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3);

void lava_wrap_block(const std::vector<uint8_t> &data);
inline void lava_op_block(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4);

void lava_wrap_rectangle(const std::vector<uint8_t> &data);
inline void lava_op_rectangle(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4);

void lava_wrap_exit(const std::vector<uint8_t> &data);
inline void lava_op_exit(uint32_t ds0);

void lava_wrap_clearscreen(const std::vector<uint8_t> &data);
inline void lava_op_clearscreen();

void lava_wrap_rand(const std::vector<uint8_t> &data);
inline uint32_t lava_op_rand();

void lava_wrap_srand(const std::vector<uint8_t> &data);
inline void lava_op_srand(uint32_t ds0);

void lava_wrap_locate(const std::vector<uint8_t> &data);
inline void lava_op_locate(uint32_t ds0, uint32_t ds1);

void lava_wrap_inkey(const std::vector<uint8_t> &data);
inline uint32_t lava_op_inkey();

void lava_wrap_point(const std::vector<uint8_t> &data);
inline void lava_op_point(uint32_t ds0, uint32_t ds1, uint32_t ds2);

void lava_wrap_line(const std::vector<uint8_t> &data);
inline void lava_op_line(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4);

void lava_wrap_circle(const std::vector<uint8_t> &data);
inline void lava_op_circle(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4);

void lava_wrap_isalpha(const std::vector<uint8_t> &data);
inline uint32_t lava_op_isalpha(uint32_t ds0);

void lava_wrap_strcat(const std::vector<uint8_t> &data);
inline void lava_op_strcat(uint32_t ds0, uint32_t ds1);

void lava_wrap_strchr(const std::vector<uint8_t> &data);
inline uint32_t lava_op_strchr(uint32_t ds0, uint32_t ds1);

void lava_wrap_strcmp(const std::vector<uint8_t> &data);
inline uint32_t lava_op_strcmp(uint32_t ds0, uint32_t ds1);

void lava_wrap_strstr(const std::vector<uint8_t> &data);
inline uint32_t lava_op_strstr(uint32_t ds0, uint32_t ds1);

void lava_wrap_memset(const std::vector<uint8_t> &data);
inline void lava_op_memset(uint32_t ds0, uint32_t ds1, uint32_t ds2);

void lava_wrap_memcpy(const std::vector<uint8_t> &data);
inline void lava_op_memcpy(uint32_t ds0, uint32_t ds1, uint32_t ds2);

void lava_wrap_fopen(const std::vector<uint8_t> &data);
inline uint32_t lava_op_fopen(uint32_t ds0, uint32_t ds1);

void lava_wrap_fclose(const std::vector<uint8_t> &data);
inline void lava_op_fclose(uint32_t ds0);

void lava_wrap_fread(const std::vector<uint8_t> &data);
inline uint32_t lava_op_fread(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3);

void lava_wrap_fwrite(const std::vector<uint8_t> &data);
inline uint32_t lava_op_fwrite(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3);

void lava_wrap_fseek(const std::vector<uint8_t> &data);
inline uint32_t lava_op_fseek(uint32_t ds0, uint32_t ds1, uint32_t ds2);

void lava_wrap_rewind(const std::vector<uint8_t> &data);
inline void lava_op_rewind(uint32_t ds0);

void lava_wrap_sprintf(const std::vector<uint8_t> &data);
inline void lava_op_sprintf(uint32_t ds0);

void lava_wrap_deletefile(const std::vector<uint8_t> &data);
inline uint32_t lava_op_deletefile(uint32_t ds0);

void lava_wrap_gettick(const std::vector<uint8_t> &data);
inline uint32_t lava_op_gettick();

void lava_wrap_checkkey(const std::vector<uint8_t> &data);
inline uint32_t lava_op_checkkey(uint32_t ds0);

void lava_wrap_crc16(const std::vector<uint8_t> &data);
inline uint32_t lava_op_crc16(uint32_t ds0, uint32_t ds1);

void lava_wrap_encrypt(const std::vector<uint8_t> &data);
inline void lava_op_encrypt(uint32_t ds0, uint32_t ds1, uint32_t ds2);

void lava_wrap_gettime(const std::vector<uint8_t> &data);
inline void lava_op_gettime(uint32_t ds0);

void lava_wrap_xdraw(const std::vector<uint8_t> &data);
inline void lava_op_xdraw(uint32_t ds0);

void lava_wrap_releasekey(const std::vector<uint8_t> &data);
inline void lava_op_releasekey(uint32_t ds0);

void lava_wrap_getblock(const std::vector<uint8_t> &data);
inline void lava_op_getblock(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4, uint32_t ds5);

void lava_wrap_setgraphmode(const std::vector<uint8_t> &data);
inline uint32_t lava_op_setgraphmode(uint32_t ds0);

void lava_wrap_setbgcolor(const std::vector<uint8_t> &data);
inline void lava_op_setbgcolor(uint32_t ds0);

void lava_wrap_setfgcolor(const std::vector<uint8_t> &data);
inline void lava_op_setfgcolor(uint32_t ds0);
