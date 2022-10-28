# Pi-clock
This is a record of my code to use an old Pi-3 and an even older EVGA monitor
to make a clock for my home office showing the next five entries from my
Google Calendar.

![Screen shot](/screenshot.jpg)

The works of clock.py are copied straight off the google API website so you'll
need to read up on getting your developer credentials but it is quite
straightforward.

I commented all the C++ code in ELI5 style so I would have a quick gtkmm
example to cut-and-stick from for future stuff. Hence I included command line
argument handling and the button/timer functions are implemented as lambdas
which is a bit OTT for a simple clock.
