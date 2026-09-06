# C1Terminal notices

C1Terminal is a standalone graphical terminal for the C1-Slim / MP-D261.
It does not contain or launch the C1ancher desktop, settings, Wi-Fi UI,
package manager, service supervisor, or application launcher.

The MP-D261 display layout, keyboard mapping, and compact 5x7 terminal font
were adapted from the GPL-3.0 C1ancher project by fwz233-RE. C1Terminal is
therefore distributed under GPL-3.0; see `LICENSE`.

The VT100 parser is `github.com/hinshun/vt10x` (MIT). PTY creation uses
`github.com/creack/pty` (MIT). Exact versions and transitive dependencies are
recorded in `go.mod`, `go.sum`, and the vendored module tree.
