#ifdef FENSTER_H
#ifdef MICROUI_H

void fenmui_init(struct fenster *fp, mu_Context * ctxp);

void fenmui_events(void);

void fenmui_draw(void);

void r_clear(mu_Color color);
void r_set_clip_rect(mu_Rect rect);
void r_draw_rect(mu_Rect rect, mu_Color color);
void r_draw_icon(int id, mu_Rect rect, mu_Color color);
void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color);

#endif
#endif

