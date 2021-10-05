package term

type Event struct {
	Key Key
}

const (
	KeyBackSpace Key = iota + 8
	KeyTab
)

const (
	KeyEnter Key = 13
)

const (
	KeySpace Key = iota + 32
	KeyExclamation
	KeyQuote
	KeyNumber
	KeyDollar
	KeyPercent
	KeyAmpersand
	KeyApostrophe
	KeyLParen
	KeyRParen
	KeyAsterisk
	KeyPlus
	KeyComma
	KeyMinus
	KeyPeriod
	KeyForwardslash
	Key0
	Key1
	Key2
	Key3
	Key4
	Key5
	Key6
	Key7
	Key8
	Key9
	KeyColon
	KeySemiColon
	KeyLess
	KeyEquals
	KeyGreater
	KeyQuestion
	KeyAt
	KeyA
	KeyB
	KeyC
	KeyD
	KeyE
	KeyF
	KeyG
	KeyH
	KeyI
	KeyJ
	KeyK
	KeyL
	KeyM
	KeyN
	KeyO
	KeyP
	KeyQ
	KeyR
	KeyS
	KeyT
	KeyU
	KeyV
	KeyW
	KeyX
	KeyY
	KeyZ
	KeyLBracket
	KeyBackslash
	KeyRBracket
	KeyCaret
	KeyUnderscore
	KeyBacktick
	Keya
	Keyb
	Keyc
	Keyd
	Keye
	Keyf
	Keyg
	Keyh
	Keyi
	Keyj
	Keyk
	Keyl
	Keym
	Keyn
	Keyo
	Keyp
	Keyq
	Keyr
	Keys
	Keyt
	Keyu
	Keyv
	Keyw
	Keyx
	Keyy
	Keyz
	KeyLCurlyBracket
	KeyBar
	KeyRCurlyBracket
	KeyTilde
	KeyDelete
)
