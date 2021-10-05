package term

var (
	Black = Color{0, 0, 0}
	White = Color{255, 255, 255}
	Red   = Color{255, 0, 0}
	Green = Color{0, 255, 0}
	Blue  = Color{0, 0, 255}
)

type Color struct {
	r uint8
	g uint8
	b uint8
}
