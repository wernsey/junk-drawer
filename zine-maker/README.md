zine-maker
==========

A simple command-line program to generate [Zine][]s in the form of [Pocketmod][] PDFs.

It takes 8 PNG or JPG files as arguments and lays them out on a single page
in a PDF file for printing.

```
./zine -pg cover.png page1.png page2.png page3.png page4.png page5.png page6.png back.png
```

* `-o` Specifies the output file. The default is `pocketmod.pdf`
* `-f` lay the page out as a [pocketfold][] instead.
* `-m margin` specifies the margin around each image in its
  panel (default 10)
* `-g` add folding guides to the output PDF
* `-p` add page numbers to the output PDF. `-pp` adds
  page numbers to the front and back covers as well
* `-T title` adds a title to the PDF metadata
* `-A author` adds an author to the PDF metadata
* `-C creator` adds a creator to the PDF metadata
* `-S subject` adds a subject to the PDF metadata
* `-K keywords` adds keywords to the PDF metadata

PDFs are generated through [libharu][].

Use HTML/CSS instead
--------------------

If you have HTML and CSS experience, you will get better control over your layout and
better print print quality if you create your zine as an HTML page and use a `@media print`
section in your stylesheet to lay out the page for printing. Another benefit is that
you get a webpage and a print zine for the same amount of work.

Here's an article that explains how you you'd go about doing that.
[Create a 🖨️ printable zine with CSS](https://dev.to/rowan_m/create-a-printable-zine-with-css-5c0c).
The https://zine-machine.glitch.me/ website that the article links to is available on the
[Internet Archive](https://web.archive.org/web/20200517011214if_/https://zine-machine.glitch.me/)

[zserge/zine][] is another project on GitHub that has the necessary stylesheets to do the same,
with some useful additions.

[zserge/zine]: https://github.com/zserge/zine

[Zine]: https://en.wikipedia.org/wiki/Zine
[Pocketmod]: https://pocketmod.com/
[libharu]: https://github.com/libharu/libharu
[pocketfold]: https://3skulls.itch.io/pocketfold