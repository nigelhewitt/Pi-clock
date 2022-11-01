//==============================================================================
// clock.cpp		Clock/calendar with Google calendar using gtkmm
//					Nigel Hewitt 2022
//
//					Feel free to cut and stick while usual disclaimers apply.
//					I had trouble finding a straight forward gtkmm example so,
//					now I have gathered bits from all over the web, it only
//					seems fair that I publish my own.
//					Tested on Pi3/4 and WSL Debian
//==============================================================================
//
// spaced with tab=4
//
// Requires:
//		sudo apt install g++ make gdb
//		sudo apt install libgtk-3-dev libgtkmm-3.0-dev
//
// Compile with the attached makefile to get the includes and the libraries
//		make
//
// Pi-Clock aka 'Pi in the Sky' is set to execute at startup
//		/home/pi/calendar/clock
// see
//		~/.config/autostart/clock.desktop
//
// 2022-01-26  initial version
// 2022-03-27  use local time not UTC
// 2022-07-27  add dow and date
// 2022-10-09  add the Google Calendar stuff
// 2022-10-12  switch to gtkmm and full on C++
// 2022-10-14  finish and fine tune as a C++ example
// 2022-10-17  add css compile error reporting
// 2022-10-26  add code for Z duration (seen new today)
// 2022-10-27  add command arguments and lambdas
// 2022-11-01  fix error reporting on token timeout
//
// For Eclipse this requires the pkg-config plugin
//   Help | Eclipse Market place
//   put pkg-config in the search box | Go
//     Install and restart
// Add to project:
//   Project | Properties | C++ Build | Settings
//     Pkg-config (initially an off screen tab to the right)
//     check gtk-3.0 and gtkmm-3.0  | Apply and continue
//
//==============================================================================

#include <gtkmm/application.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <gtkmm/fixed.h>
#include <gtkmm/main.h>
#include <gtkmm/cssprovider.h>
#include <glibmm/main.h>
#include <iostream>

// Define some CSS so we can set colours and fonts and stuff
// I break it into lines with \n so we get useful error messages
// if it fails to compile

static const char *css =
"window {\n"							// top level Gtk::Window
" background: black;\n"
" font-family: terminal;\n"
" }\n"
"button {\n"							// all Gtk::Buttons
" color: white;\n"
" background: blue;\n"
" font-size: 50px;\n"
" border-width: 5px;\n"
" border-radius: 5px;\n"
" border-color: white\n"
" }\n"
"label#aval {\n"						// distinguish Gtk::Labels by name
" color: white;\n"
" font-size: 250px\n"
" }\n"
"label#bval {\n"
" color: lawngreen;\n"
" font-size: 100px\n"
" }\n"
"label#sval1 {\n"
" color: red;\n"
" font-size: 60px\n"
" }\n"
"label#sval2 {\n"
" color: royalblue;\n"
" font-size: 60px\n"
" }\n"
;

// Now the class that defines our main window
// I have coded it with the functions 'inline' C# style

class CLOCK : public Gtk::Window {
protected:
	// As there only one instance of CLOCK I can subvert the static/dynamic vibe
	inline static CLOCK* self{ nullptr };

	// Member widgets:
	Gtk::Fixed fixed;				// a widget container with fixed coordinates
	Gtk::Button close, refresh;		// buttons
	Gtk::Label time, day, date;		// blocks of text
	Gtk::Label slot[5];				// more text for the calendar entries

	bool bTest{ false };			// used when testing

public:
	CLOCK() = delete;							// no default constructor
	CLOCK(Glib::RefPtr<Gtk::Application> app){	// the constructor for the window
		self = this;
		set_title("Pi-Clock");
		set_border_width(10);

		// Select a 'fixed' container so we get absolute coordinates.
		// Beware: the basic Gtk::Window can only contain one widget but this
		// is a widget that contains lots of widgets
		// My old screen is a 1440 x 900 - old but that's why it's on the clock
		fixed.set_size_request(1440-30, 900-52);  // minimum (preferred) so
												  // screen size less borders,
												  // toolbar and title et al
		add(fixed);			// put the Gtk::Fixed in the Gtk::Window

		// Arrange for the CSS to do colours and fonts
		auto context = get_style_context();
		auto provider = Gtk::CssProvider::create();
		try{
			provider->load_from_data(css);
		}
		catch(const Gtk::CssProviderError& e){
			// If there is a syntax error the gtkmm code helpfully eats the
			// error message. So, if it fails, recompile it with a more basal
			// function that actually throws with a useful error description
			// with line and column numbers.
			// It will still throw but at least you get a clue to fix it
			if(e.code()==Gtk::CssProviderError::SYNTAX){
				GtkCssProvider *provider = gtk_css_provider_new();
				gtk_css_provider_load_from_data(provider, css, -1, nullptr);
			}
			std::cout << "CssProviderError: error " << e.code() << std::endl;
			exit(1);
		}
		context->add_provider_for_screen(Gdk::Screen::get_default(),
							provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

		// Give the labels CSS names so we can distinguish them
		time.set_name("aval");			// ie: use label#aval
		day.set_name("bval");
		date.set_name("bval");
		for(int i=0; i<5; ++i)
			slot[i].set_name("sval1");

		// Connect the buttons to their service routines as lambdas
		close.signal_clicked().connect([this]{ return Gtk::Window::close(); });
		refresh.signal_clicked().connect([this]{ Ticks = 12; });

		// And the command line argument receiver
		// more messy as it is a static
		app->signal_command_line().connect(
			[app](const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line){
				int argc = 0;
				char** argv = command_line->get_arguments(argc);
				self->do_command(argc, argv);	// call a member function of CLOCK

				app->activate();	// beware: this is in the default
									// on_command() so we must do it too
				return 0;
			}, false);

		// Set the buttons' texts and put them into the fixed container
		close.set_label("Close");
		refresh.set_label("Refresh");
		fixed.put(close,   25,   15);
		fixed.put(refresh, 1140, 15);

		// Put the labels into the container
		fixed.put(time, 100,  70);
		fixed.put(day,  95,  320);
		fixed.put(date, 720, 320);
		for(int i=0; i<5; ++i)
			fixed.put(slot[i], 60, 455+i*70);

		// The final step is to display all these newly created widgets...
		show_all_children();

		// Make a timer to call CLOCK::tick() every 1000mS
		// I'll use a lambda again to save a layer of indirection
		Glib::signal_timeout().connect([this]() { return this->tick(); }, 1000);
	}
	virtual ~CLOCK(){}		// default clean-ups only

	// receive the command args
	void do_command(int argc, char* argv[])
	{
		for(int i=0; i<argc; ++i){
			if(strcmp(argv[i], "-t")==0)
				bTest = true;
		}
	}

	// Ticker at 1 per second
	int Ticks{25};			// delay the first fetch for fifteen seconds
	int Retries{0};			// limit the fast retries
	char today[12]{};		// used to colour the lines for 'today'

	// Update the time, day and date
	int oldDOW{9};			// trigger the refresh of day oriented stuff

	void setDisplay()
	{
		char temp[30];
		time_t now;
		::time(&now);						// get UTC
		tm *t = localtime(&now);			// convert to BST or whatever

		sprintf(temp, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
		time.set_text(temp);

		// the rest only changes if the day changes
		if(t->tm_wday != oldDOW){
			oldDOW = t->tm_wday;
			const char* dow[] = { "Sunday",   "Monday", "Tuesday", "Wednesday",
								  "Thursday", "Friday", "Saturday"  };
			day.set_text(dow[t->tm_wday]);

			sprintf(temp, "%02d-%02d-%04d", t->tm_mday, t->tm_mon+1, 1900+t->tm_year);
			date.set_text(temp);

			// Make a value to compare to the Google calendar stuff:
			sprintf(today, "%04d-%02d-%02d", 1900+t->tm_year, t->tm_mon+1, t->tm_mday);
		}
	}

	// Update the calendar display
	void setCalendar()
	{
		// The events file has four sorts of entries, all day, timed and errors
		// 2022-10-13 Exercise\n
		// 2022-10-13T12:00:00+01:00 Lunch with Robin\n
		// 2022-11-01T21:00:00Z Recycling\n             (first seen 26/10/2022)
		// * something bad happened\n
		// Its stderr output are sent to response.edc so we can try
		// and fail responsibly

#define CALDIR	"/home/pi/calendar"
		const char* eventsFile   = CALDIR "/events.txt";
		const char* responseFile = CALDIR "/response.edc";

		if(--Ticks==10 && !bTest){	// at 10 seconds to go run the calendar
			if(fork()==0){			// go multi-threaded
				chdir(CALDIR);
				unlink(responseFile);
				unlink(eventsFile);
				char command[100];
				sprintf(command, "python clock.py 2> %s", responseFile);
				system(command);
				exit(0);		// kill the forked thread
			}
		}
		if(Ticks<=0){
			Ticks = bTest ? 60 : 60*60;		// reset for one hour
			int i=0;
			FILE* f = fopen(eventsFile, "r");
			if(f){
				char text1[200], text2[200];
				for(; i<5 && fgets(text1, sizeof(text1), f); ++i){
					// first process errors from clock.py
					if(text1[0]=='*'){
						int n = strlen(text1);				// tidy
						if(text1[n-1]=='\n') text1[n-1] = 0;
						slot[i].set_text(text1);
					}
					else{
						// copy the date to the output
						int j=0, k=0;
						while(j<10) text2[k++] = text1[j++];

						// is there a time?
						if(text1[j++]!='T'){				// no
							strcpy(text2+k, " all day  ");
							k = strlen(text2);
						}
						else{
							text2[k++] = ' ';
							// copy the start time but discard the duration
							while(j<19) text2[k++] = text1[j++];
							text2[k++] = ' ';
							// is it a '+' time or a 'Z' time?
							if(text1[j]=='+') j += 7;
							else              j += 2;
						}
						// copy the event text
						while(text1[j] && text1[j]!='\n')
							text2[k++] = text1[j++];
						text2[k] = 0;

						// check the date for today and if so use red text
						const char* fg = "sval1";			// red
						if(strncmp(text2, today, 10))
							fg = "sval2";					// royal blue
						slot[i].set_name(fg);
						slot[i].set_text(text2);
					}
				}
				Retries = 0;
				fclose(f);
			}
			else{				// if the events file failed to open
				// If it fails a couple of times retry but if it's stuck revert
				// to the one hour schedule.
				if(++Retries<4)
					Ticks = 60*2;	// give it two minutes and then try again
				FILE* f2 = fopen(responseFile, "r");
				if(f2){
					char buffer[200];
					while(i==0 && fgets(buffer, sizeof(buffer), f2)!=nullptr){
						if(strstr(buffer, "Token has been expired")!=nullptr){
							slot[i].set_text("** Token refresh time **");
							slot[i++].set_name("sval1");		// red
							slot[i].set_text("   cd calendar");
							slot[i++].set_name("sval1");		// red
							slot[i].set_text("   rm token.json");
							slot[i++].set_name("sval1");		// red
							slot[i].set_text("   python clock.py");
							slot[i++].set_name("sval1");		// red
							slot[i].set_text("   wait for the browser and agree");
							slot[i++].set_name("sval1");		// red
						}
					}
					fclose(f2);
				}
			}
			if(i==0){						// response file failed too
				slot[i].set_name("sval1");	// red
				slot[i++].set_text("** Data failed to fetch **");
			}
			for( ; i<5; ++i){			// blank the rest of the display
				slot[i].set_name("sval2");
				slot[i].set_text("**");
			}
		}
	}
	bool tick()
	{
		setDisplay();
		setCalendar();
		return true;
	}
};


int main(int argc, char *argv[])
{
	// Command line arguments are a pain under gtkmm so I will try to explain.
	// We add the APPLICATION_HANDLES_COMMAND_LINE flag so we get sent the args
	// then we hook up a receiver callback in CLOCK to handle them
	// This way gtkmm gets a first look at the args and acts on and takes out
	// those that belong to it and then passes the rest on down to us.

	auto app = Gtk::Application::create(argc, argv, "clock.app",
							Gio::APPLICATION_HANDLES_COMMAND_LINE);

	CLOCK Clock(app);

	// Show the window and returns when it is closed.
	return app->run(Clock);
}
