struct slice {
	uint8_t *buf;
	int cap;
	int len;
};

void slice_init(struct slice *s, int cap)
{
	s->len = 0;
	s->cap = cap;
	s->buf = (void *)malloc(cap);
}

void slice_free(struct slice *s)
{
	s->len = 0;
	s->cap = 0;
	free(s->buf);
}

void slice_append(struct slice *s, void *data, int size)
{
	int cap = s->cap;

	while (cap - s->len < size) {
		cap *= 2;
	}

	if (cap > s->cap) {
		uint8_t *buf = malloc(cap);
		memcpy(buf, s->buf, s->len);
		free(s->buf);
		s->buf = buf;
		s->cap = cap;
	}

	memcpy(s->buf + s->len, data, size);
	s->len += size;
}
