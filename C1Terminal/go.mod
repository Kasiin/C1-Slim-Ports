module c1terminal

go 1.25.0

require (
	github.com/creack/pty v1.1.24
	github.com/hinshun/vt10x v0.0.0-20220301184237-5011da428d02
)

replace github.com/creack/pty => ./third_party/pty-1.1.24

replace github.com/hinshun/vt10x => ./third_party/vt10x-5011da428d02
