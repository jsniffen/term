struct finn {
	bool running;
};

void finn_init(struct finn *f)
{
	f->running = true;
}

void finn_update_and_render(struct finn *f, struct terminal *t, union event *e)
{

	if (e->keyboard.alt && (e->keyboard.key == Keyq || e->keyboard.key == KeyQ)) {
		f->running = false;
	}

	if (e->keyboard.key == Keys) {
		show_cursor_terminal(t);
	}
	if (e->keyboard.key == Keye) {
		hide_cursor_terminal(t);
	}

	modal(t, 0.75);
}
