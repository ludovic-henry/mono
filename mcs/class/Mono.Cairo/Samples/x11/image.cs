//
//
//	Mono.Cairo drawing samples using X11 as drawing surface
//	Autor: Hisham Mardam Bey <hisham@hisham.cc>
//

//
// Copyright (C) 2005 Novell, Inc (http://www.novell.com)
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

using System;
using System.Reflection;
using System.Runtime.InteropServices;
using Cairo;

public class X11Test
{
        static readonly double  M_PI = 3.14159265358979323846;
	
        static void draw (Cairo.Context gr, int width, int height)
	{
		int w, h;
		ImageSurface image;
		
		gr.Scale (width, height);
		gr.LineWidth = 0.04;
		
		image = new ImageSurface ("data/e.png");
		w = image.Width;
		h = image.Height;
		
		gr.Translate (0.5, 0.5);
		gr.Rotate (45* M_PI/180);
		gr.Scale  (1.0/w, 1.0/h);
		gr.Translate (-0.5*w, -0.5*h);
		
		image.Show (gr, 0, 0);
		image.Destroy ();
	}
	
	
	
	static void Main (string [] args)
	{
		Window win = new Window (500, 500);
		
		win.Show ();
		
		Cairo.XlibSurface s = new Cairo.XlibSurface (win.Display,
			       win.XWindow,
			       X11.XDefaultVisual (win.Display, win.Screen),
			       (int)win.Width, (int)win.Height);

		
		Cairo.Context g = new Cairo.Context (s);
		
		draw (g, 500, 500);
		
		IntPtr xev = new IntPtr ();
		
		while (true) {			
			X11.XNextEvent (win.Display, xev);
		}		
	}
}
