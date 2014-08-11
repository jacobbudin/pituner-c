/* src/pituner.c */
void ptn_error(const char *error);
void ptn_debug(const char *format, ...);
struct ptn_display ptn_get_stream_info(void);
void ptn_update_display(struct ptn_display *info);
void ptn_read_config(void);
void ptn_reset_station(void);
void ptn_free(void);
int ptn_check_keyboard(void);
int ptn_check_dial(void);
void ptn_load_station(void);
void ptn_play_station(void);
void ptn_stop_station(void);
void ptn_change_station(int offset);
int main(int argc, char *argv[]);
