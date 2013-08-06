Documentation
=============

Directory doc contains the documentation.

The documentation is in asciidoc format, which means it is perfectly
readable as a text file. To convert to other formats, you have to have asciidoc
tools installed (see http://asciidoc.org/INSTALL.html).

Converting to XHTML:

	a2x -f xhtml jabsocket.ad

Converting to PDF:

	a2x -f pdf jabsocket.ad

In the doc directory there is a Makefile that can do this. Just type
	make

and it produces both file jabsocket.html and jabsocket.pdf.

