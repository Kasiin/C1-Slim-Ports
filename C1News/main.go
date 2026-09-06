package main

import (
	"context"
	"fmt"
	"os"
)

func main() {
	if e := command(); e != nil {
		fmt.Fprintln(os.Stderr, e)
		os.Exit(1)
	}
}
func command() error {
	args := os.Args[1:]
	dir := "/storage/c1news/cache"
	if len(args) == 1 && args[0] == "--version" {
		fmt.Println("C1News 1.2.0 / pixel-aligned 15px reading / SSPAI public text")
		return nil
	}
	if len(args) == 2 && args[0] == "--refresh-only" {
		fail := 0
		for _, src := range sources {
			f, e := refreshFeed(context.Background(), src, args[1])
			if e == nil {
				e = saveFeed(args[1], f)
			}
			if e != nil {
				fmt.Println(src.ID, e)
				fail++
			} else {
				fmt.Printf("%s: %d articles, %s\n", src.ID, len(f.Articles), f.Articles[0].Date)
			}
		}
		if fail > 0 {
			return fmt.Errorf("%d sources failed; old cache preserved", fail)
		}
		return nil
	}
	if len(args) == 4 && args[0] == "--preview" {
		u := UI{}
		for i, s := range sources {
			u.Feeds[i], _ = loadFeed(args[1], s.ID)
			u.Status[i] = cacheStatus(u.Feeds[i])
		}
		switch args[2] {
		case "home":
		case "list":
			u.View = 1
		case "article":
			u.openArticle()
		case "sspai":
			u.Source = 1
			u.openArticle()
		case "ifanr":
			u.Source = 2
			u.openArticle()
		default:
			return fmt.Errorf("unknown preview")
		}
		f := u.render()
		return os.WriteFile(args[3], f[:], 0644)
	}
	if len(args) != 0 {
		return fmt.Errorf("usage: c1news [--version | --refresh-only DIR | --preview DIR PAGE FILE]")
	}
	return runDevice(dir)
}
