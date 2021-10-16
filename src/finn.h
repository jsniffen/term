struct finn {
	bool running;
	bool show_modal;
};

void finn_init(struct finn *f)
{
	f->running = true;
	f->show_modal = false;
}
void finn_update_and_render(struct finn *f, struct terminal *t, union event *e)
{
	if (e->keyboard.alt && (e->keyboard.key == Keyq || e->keyboard.key == KeyQ)) {
		f->running = false;
	}

	if (e->keyboard.key == Keys) {
		f->show_modal = true;
	}
	if (e->keyboard.key == Keye) {
		f->show_modal = false;
	}

	status_bar(t, "win Newcol Kill Putall Dump Exit");

	window(t, 3, 0);
	window(t, 3, 1);
	window(t, 3, 2);

	if (f->show_modal) modal(t, 0.5);
}
