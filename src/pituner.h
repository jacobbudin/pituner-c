/* src/pituner.c */
void ptn_error(const char *error);
void ptn_debug(const char *format, ...);
int ptn_update_stream(void);
void ptn_update_display(void);
void ptn_read_config(void);
void ptn_reset_station(void);
void ptn_free(void);
int ptn_check_keyboard(void);
void ptn_reset_dial(void);
int ptn_check_dial(void);
void ptn_load_station(void);
void ptn_play_station(void);
void ptn_stop_station(void);
int ptn_change_station(int offset);
int main(int argc, char *argv[]);
