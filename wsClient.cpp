#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <ncurses.h>
#include <sstream>
#include "function.cpp"

using tcp = boost::asio::ip::tcp;



int ymax;
int xmax;
int chatwinx;
int chatwiny;
WINDOW *win;
WINDOW *upwin;
std::vector<std::string> asciiArt = {
        "            ___     ___     ___     ___     ___             ___    _  _     ___    _____  ",
        "    o O O  /   \\   / __|   / __|   |_ _|   |_ _|           / __|  | || |   /   \\  |_   _| ",
        "   o       | - |   \\__ \\  | (__     | |     | |           | (__   | __ |   | - |    | |   ",
        "  TS__[O]  |_|_|   |___/   \\___|   |___|   |___|   _____   \\___|  |_||_|   |_|_|   _|_|_  ",
        " {======|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"| ",
        "./o--000'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-'"
    };





void draw_chat()
{
    box(win, 0, 0);
    wattron(win, A_BOLD);
    mvwprintw(win, 0, 1, "CHAT");
    wattroff(win, A_BOLD);
}

void draw_upwin()
{
werase(upwin);
wattron(upwin, A_BOLD);
    for (int i = 0; i < asciiArt.size(); ++i){
        mvwprintw(upwin, i+2, 1, "%s", asciiArt[i].c_str());
    }
wattroff(upwin, A_BOLD);

box(upwin, 0, 0);
wrefresh(upwin);
}




void handle_sigwinch(int sig) {
    wmove(win, 0, 0);
    wclrtoeol(win);
    wmove(win, chatwiny-1, 0);
    wclrtoeol(win);
    for (int y = 0; y < chatwiny; ++y) {
        mvwaddch(win, y, 0, ' ');
        mvwaddch(win, y, chatwinx-1, ' ');
    }
    wrefresh(win);
    endwin();

    getmaxyx(stdscr, ymax, xmax);

    chatwinx =  0.75*(xmax-5);
    chatwiny = 0.8*(ymax-9);
    wresize(win, chatwiny, chatwinx);
    wmove(win, 0, 0);
    box(win, 0, 0);
    wrefresh(win);
}


void asyncWrite(boost::beast::websocket::stream<tcp::socket>& ws, const std::string& message, boost::asio::strand<boost::asio::io_context::executor_type>& strand) {
    ws.async_write(
        boost::asio::buffer(message),
        boost::asio::bind_executor(strand, [&ws](boost::beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cerr << "write Error in wsClient: " << ec << " " << ec.message() << std::endl;
                return;
            }
        }));
}


// Data structure to store the message (store username, color, input etc.)
class messageStorage{
    public:
    char type; // type of the message, 'c' - connection (input is empty), 'd' - disconnection (input is empty), 'm' - message
    std::string username;
    int color;
    std::string input;
    messageStorage(char T, std::string name, int c, std::string in = ""): type(T), username(name), color(c), input(in){
        if (type == 'c'){
           input = username + " has connected.";
        }
        if (type == 'd')
           input = username + " has disconnected.";
    }
};

class ChatWindow {
    public:
    WINDOW* win_;
    std::vector<messageStorage*> messages;
    int currentline = 1;
    boost::beast::websocket::stream<tcp::socket>& ws_;
    boost::asio::strand<boost::asio::io_context::executor_type>& strand_;
    std::string clientname;

    ChatWindow(WINDOW* w, boost::beast::websocket::stream<tcp::socket>& ws, boost::asio::strand<boost::asio::io_context::executor_type>& strand, std::string name)
    : win_(w), ws_(ws), strand_(strand), clientname(name){

    }
    void saveline(std::string input, std::string username, int color){
        wmove(win, currentline, 0);
        wclrtoeol(win); // clear the line
        messageStorage* tmp = new messageStorage('m' ,username, color, input);
        messages.push_back(tmp);

    }
    void writeline(std::string input, std::string username, int color){
        saveline(input, username, color);
        std::stringstream ss;
        ss << "m;" << username << ";" << color << ";" << input;
        std::string s = ss.str();
        asyncWrite(ws_, ss.str(), strand_);
    }
    //function that will print the all the messages to the chat win
    void print(int l = 0){
        int maxchar = chatwinx - 3;
        int lines = chatwiny - 3 - l; // lines capacity, l is if we want to leave more space for text input (eg. in upshift func)
        // we want to go through our messages from back and stop when our line capacity goes out
        int j = messages.size() -1;
        int lines_overflow; // we will know when the last msg is not fully in the chat window by this int being negative.

        for (lines_overflow = lines; lines_overflow>0 && j >= 0;){
            messageStorage* tmp = messages[j--];
            lines_overflow -= (1 + tmp->input.length()/(maxchar - 1 - aftername(tmp->username)));
        }
        j++; // j is -1 from the first loop but we need it to be 0
        int printline = 1;
        for (; j<messages.size(); ++j){
            messageStorage* tmp = messages[j];
            int i = 0;
            int char_per_line = maxchar - 1 - aftername(tmp->username);
            if (lines_overflow < 0) {
                i += std::abs(lines_overflow * char_per_line);
                lines_overflow = 0;
            } else {
                wmove(win, printline, 1);
                wclrtoeol(win);
                wattron(win, COLOR_PAIR(tmp->color));
                mvwprintw(win, printline, 1, "[%s]: ", tmp->username.c_str());
                wattroff(win, COLOR_PAIR(tmp->color));
            }

            for(; i<tmp->input.length(); i+= char_per_line ) {
                if(i>0){
                wmove(win, printline, 1);
                wclrtoeol(win);
                }
                int endidx = std::min(static_cast<int>(tmp->input.length()), i+ char_per_line);
                mvwprintw(win, printline, aftername(tmp->username), "%s", tmp->input.substr(i, endidx-i).c_str());
                ++printline;
            }

            }
            currentline = printline;
        //draw_chat();
        //  wnoutrefresh(win);



    }

    void upshift(int value){
        print(value);
    }

};

void INPUTthread(std::queue<int>& inputQueue, std::mutex& queueMutex, bool& loop){
    int c;
    while (loop){
        c = wgetch(win);
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            inputQueue.push(c);
        }
    }
}


int UIthread(ChatWindow* chatwin) {

    //WINDOWS
    getmaxyx(stdscr, ymax, xmax);
    chatwinx =  0.75*(xmax-5);
    chatwiny = 0.8*(ymax-9);
    win = newwin(chatwiny, chatwinx, 10, 5);
    upwin = newwin(9, 92, 0, 0);
    keypad(win, TRUE);


    //COLOR
    if (has_colors() == FALSE) {
        endwin();
        std::cout << "Your terminal does not support color.\n";
        return 1;
    }
    start_color();
    init_color(COLOR_YELLOW, 1000, 1000, 0);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    //SIGNALS
    //signal(SIGWINCH, handle_sigwinch);

    std::string username = chatwin->clientname;
    int prefix = username.length() + 5; // length of the prefix eg. '[Ales.h] % ' +1 to land on the next empty char
    int message_change; // int to know if the number of messages has changed

    // printing ASCII art
    wattron(upwin, A_BOLD);
    for (int i = 0; i < asciiArt.size(); ++i){
        mvwprintw(upwin, i+2, 1, "%s", asciiArt[i].c_str());
    }
    wattroff(upwin, A_BOLD);

    // init of the window
    box(upwin, 0, 0);
    draw_chat();
    wrefresh(upwin);
    bool run = true;

    std::queue<int> inputQueue;
    std::mutex queueMutex;
    std::thread inputThreadObj([&inputQueue, &queueMutex, &run]{INPUTthread(inputQueue, queueMutex, run); });
    int reprint = 0;
    while (run) {
        //wclear(win);

        wattron(win ,COLOR_PAIR(1));
        mvwprintw(win, chatwin->currentline, 1, "[%s] %% ", username.c_str());
        wattroff(win, COLOR_PAIR(1));
        int currentchar = prefix + 1;
        wmove(win, chatwin->currentline, currentchar);
        std::string input;
        int c;
        wrefresh(win);

        bool input_loop = true;

        int lineoffset = 0;
        int maxchar;
        int number_of_lines = 0;

        bool newinput = false; // bools for if statement at the end, if there is new input char reprint the input lines
        bool newmess = false;  // if new foreign message reprint the input lines

        while(input_loop){
            wmove(win, chatwin->currentline+lineoffset, currentchar);
            wrefresh(win);
            if(!inputQueue.empty()){ // if there is new input
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    c = inputQueue.front();
                    inputQueue.pop();
                }
                newinput = true;
                if(c != '\n'){ // if user presses Enter, leave input loop
                maxchar = chatwinx - 3;
                wmove(upwin, 1, 1);
                wclrtoeol(upwin);
                wprintw(upwin, "%d", c);
                wmove(upwin, 1, 5);
                wprintw(upwin, "%d", chatwin->currentline);
                wrefresh(upwin);
                if (c == KEY_UP) {
                    // history up
                 } else if (c == KEY_DOWN) {
                // history down
               } else if (c == KEY_LEFT) {
                    currentchar = std::max(prefix+1, --currentchar);
              } else if (c == KEY_RIGHT) {
                    if (lineoffset == number_of_lines) {
                        currentchar = std::min(static_cast<int>(input.size()%(maxchar-prefix))+1+prefix, ++currentchar);
                    } else if (currentchar == maxchar && lineoffset<number_of_lines){
                        ++lineoffset;
                      currentchar = prefix + 1;
                    } else {
                     currentchar = std::min(static_cast<int>(input.size())+1+prefix, ++currentchar);
                    }
              } else if (c == KEY_RESIZE) {
                    int ch = currentchar-prefix - 1 + (lineoffset * (maxchar - prefix-1));
                    wmove(win, chatwin->currentline, 1);

                    wclrtoeol(win);
                    handle_sigwinch(SIGWINCH);
                    number_of_lines = input.length()/(maxchar-prefix-1); // 0 when on the first line
                    chatwin->upshift(number_of_lines);
                    wattron(win ,COLOR_PAIR(1));
                    mvwprintw(win, chatwin->currentline, 1, "[%s] %% ", username.c_str());
                    wattroff(win, COLOR_PAIR(1));
                    maxchar = chatwinx - 3;
                    currentchar = prefix + 1 + ch%(maxchar - prefix - 1);
                    lineoffset = ch/(maxchar - prefix-1);
                    draw_upwin();
                } else if (c == 127 || c == 263 || c == KEY_BACKSPACE) {
                    if(input.length() == 0 || currentchar == prefix + 1){
                        // do nothing because there is nothing to delete
                        if(lineoffset > 0){
                            --lineoffset;
                            currentchar = maxchar;
                      }
                    } else {
                         input.erase(currentchar-prefix-2+lineoffset*(maxchar-prefix), 1);
                         currentchar--;
                        }
                } else {
                    wmove(win, chatwin->currentline+lineoffset,  currentchar);

                    try{
                        input.insert(currentchar-prefix - 1 + (lineoffset * (maxchar - prefix-1)), 1 ,static_cast<char>(c));
                    } catch(std::exception const& ec){
                        std::cerr << "inserting to input: " << ec.what() << std::endl;
                    }
                    if (currentchar >= maxchar) {
                        ++lineoffset;
                        currentchar = prefix + 1;
                }
                    currentchar++;

            }


                } else { // end input loop because Enter was pressed
                    input_loop = false;
                    break;
                }
            } // no new input => just print and refresh for foreign messages

            if(message_change < chatwin->messages.size()) {
              //  werase(win);
                chatwin->print();
                //wrefresh(win);
                newmess = true;
            }
            if(newmess || newinput){
                int lines_comp = number_of_lines; // int to know if number_of_lines changed
                number_of_lines = input.length()/(maxchar-prefix-1); // 0 when on the first line
                if(chatwin->currentline + lines_comp == chatwiny-2 && number_of_lines > lines_comp) {
                    wmove(win, chatwin->currentline, 1);
                    wclrtoeol(win);
                    chatwin->upshift(number_of_lines);
                    wattron(win ,COLOR_PAIR(1));
                    mvwprintw(win, chatwin->currentline, 1, "[%s] %% ", username.c_str());
                    wattroff(win, COLOR_PAIR(1));
                }
                std::vector<int> tmp;
                for (int i = 0; i <= number_of_lines; ++i){
                    wmove(win, chatwin->currentline + i, prefix + 1);
                    wclrtoeol(win);
                    int startidx = i * (maxchar - prefix - 1);
                    int endidx = std::min(startidx + maxchar-prefix - 1 , static_cast<int>(input.length()));
                    try{
                        wprintw(win, "%s", input.substr(startidx, endidx-startidx).c_str());
                    } catch(std::exception const& ec){
                    std::cerr << "substringing" << ec.what() << std::endl;
                    }
                }
        //for(int j = 0; j < tmp.size(); ++j){
        //    wmove(upwin, 1, 10+(j*5));
        //    wprintw(upwin, "%d", tmp[j]);
        //}
            reprint++;
            wmove(upwin, 1, 20);
            wprintw(upwin, "%d", reprint);
            draw_chat();
            wattron(win ,COLOR_PAIR(1));
            mvwprintw(win, chatwin->currentline, 1, "[%s] %% ", username.c_str());
            wattroff(win, COLOR_PAIR(1));
            wrefresh(upwin);
            wmove(win, chatwin->currentline+lineoffset,  currentchar);
            wrefresh(win);
            newinput = false;
            newmess = false;
            } // end of reprint input lines if statement
        message_change = chatwin->messages.size();

        } // end of input loop
        wrefresh(win);
        wmove(win, chatwin->currentline, 0);
        wclrtoeol(win);
        if(input.length()>0){
            chatwin->writeline(input, username, 1);
        }
        //werase(win);
        //chatwin->print();

        wmove(upwin, 1, 5);
        wprintw(upwin, "%d", message_change);
        wrefresh(upwin);
        //draw_chat();
        wmove(win, chatwin->currentline, prefix + input.length());
        wrefresh(win);
    }
    if(inputThreadObj.joinable()){
        inputThreadObj.join();
    }

    delwin(win);
    delwin(upwin);
    endwin();
    return 0;
}


void read(boost::beast::flat_buffer& buffer, boost::beast::websocket::stream<tcp::socket>& ws, ChatWindow* chatwin_, boost::asio::strand<boost::asio::io_context::executor_type>& strand){

    ws.async_read(buffer,
    boost::asio::bind_executor(strand ,[&ws, &buffer, chatwin_, &strand](boost::beast::error_code ec, std::size_t byte_transfered){
        if(ec == boost::beast::websocket::error::closed) {
            ws.close(boost::beast::websocket::close_code::normal);
            return;
                }
        if(ec) {
            std::cerr << "read Error in wsClient: " << ec << ec.message() << std::endl;
            return;
            }
        std::string output = boost::beast::buffers_to_string(buffer.data());
        std::vector<std::string> mess = split(output, ';');
        if(mess[1] != chatwin_->clientname) { // we dont want to save messages that are sent from the current client
            chatwin_->saveline(mess[3], mess[1], std::stoi(mess[2]));
        }
        //chatwin_->print();
        buffer.consume(buffer.size());


        read(buffer, ws, chatwin_, strand);
        }));

}

int main(int argc, char** argv ){
    try{
    if(argc != 3){
        std::cerr << "Usage: ./wsClient <ip-address> <port>" << std::endl;
        return EXIT_FAILURE;
    }

    boost::asio::ip::address const address = boost::asio::ip::make_address(argv[1]);
    unsigned short const port = static_cast<unsigned short>(std::atoi(argv[2]));
    std::string username;

    std::cout << "Your username: " << std::endl;
    std::cin >> username;

    boost::asio::io_context ioc;

    tcp::resolver resolver{ioc};
    boost::beast::websocket::stream<tcp::socket> ws{ioc}; // local websocket object
    tcp::endpoint endpoint(address, port);
    auto const results = resolver.resolve(endpoint);

    // create strands for read and write
    boost::asio::strand<boost::asio::io_context::executor_type> readStrand(ioc.get_executor());
    boost::asio::strand<boost::asio::io_context::executor_type> writeStrand(ioc.get_executor());

    //INIT NCURSES
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();


    // tcp connection
    boost::asio::connect(ws.next_layer(), results);

    ws.handshake(argv[2], "/");

    boost::beast::flat_buffer buffer;


    ChatWindow chatwin(win, ws, writeStrand, username);

    std::thread UI([&chatwin, username]{ UIthread(&chatwin); });

    read(buffer, ws, &chatwin, readStrand);

    ioc.run();

    if (UI.joinable()) {
            UI.join();
        }

    }
    catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
