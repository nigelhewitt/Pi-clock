# Pi-clock

This is a record of my code to use an old Pi-3 and an even older EVGA monitor
to make a clock for my home office showing the time and date and the next five
entries from my Google Calendar.

![Screen shot](/screenshot.jpg)

The works of clock.py are copied straight off the google API website with a
file output added so you'll need to read up on getting your developer
credentials but it is free and quite straightforward.

I commented all the C++ code in ELI5 style so I would have a quick gtkmm
example to go to for future projects. Also I included the command line argument
handling stuff and implemented the button/timer functions as lambdas which
might be a bit OTT for a simple clock.

The only annoyance is that the Google user token only lasts about a week. I
suspect this can be fixed but haven't bothered yet. Most of the commits are me
tweaking the spelling...