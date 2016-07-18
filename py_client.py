#!/bin/python

import sys
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

class Linamp(Gtk.Window):

    def __init__(self):
        Gtk.Window.__init__(self, title='Linamp')

        self.box = Gtk.Box(spacing=6)
        self.add(self.box)

        self.button1 = Gtk.Button(label="Play")
        self.button1.connect("clicked", self.on_button1_clicked)
        self.box.pack_start(self.button1, True, True, 0)

        self.button2 = Gtk.Button(label="Pause")
        self.button2.connect("clicked", self.on_button2_clicked)
        self.box.pack_start(self.button2, True, True, 0)

    def on_button1_clicked(self, widget):
        print('PLAY')
        sys.stdout.flush()

    def on_button2_clicked(self, widget):
        print('PAUS')
        sys.stdout.flush()

win = Linamp()
win.connect("delete-event", Gtk.main_quit)
win.show_all()
Gtk.main()
