#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <ncurses.h>
#include <sstream>


using tcp = boost::asio::ip::tcp;

std::vector<std::string> split(const std::string& str, char separator) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = str.find(separator, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(str.substr(start));
    return tokens;
}


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


int aftername(std::string name, int border = 4){
    return (name.length() + border + 1);
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


void asyncWrite(boost::beast::websocket::stream<tcp::socket>& ws, const std::string& message) {
    ws.async_write(
        boost::asio::buffer(message),
        [&ws](boost::beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cerr << "write Error in wsClient: " << ec << " " << ec.message() << std::endl;
                return;
            }
        }
    );
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
    WINDOW* win;
    std::vector<messageStorage*> messages;
    int currentline = 1;
    boost::beast::websocket::stream<tcp::socket>* ws_;

    ChatWindow(WINDOW* w): win(w){

    }

    void writeline(std::string input, std::string username, int color){
        wmove(win, currentline, 0);
        wclrtoeol(win); // clear the line
        // wattron(win, COLOR_PAIR(color));
        // mvwprintw(win, currentline-1, 1, "[%s]: ", username.c_str());
        // wattroff(win, COLOR_PAIR(color));
        // mvwprintw(win, currentline-1, aftername(username, 4), "%s", input.c_str());
        messageStorage* tmp = new messageStorage('m' ,username, color, input);
        messages.push_back(tmp);

        std::stringstream ss;
        ss << "m;" << username << ";" << color << ";" << input;
        asyncWrite(ws_, );
        
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

            
    }
    
    void upshift(int value){
        print(value);
    }

};


int UIthread(ChatWindow* chatwin) {
    //INIT NCURSES
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();

    //WINDOWS
    getmaxyx(stdscr, ymax, xmax);
    chatwinx =  0.75*(xmax-5);
    chatwiny = 0.8*(ymax-9);
    win = newwin(chatwiny, chatwinx, 10, 5);
    upwin = newwin(9, 92, 0, 0);
    keypad(win, TRUE); 
    int f = 0;

  
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

    std::string username = "Ales.h";
    int prefix = username.length() + 5; // length of the prefix eg. '[Ales.h] % ' +1 to land on the next empty char

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
    while (true) {
        //wclear(win);    

        wattron(win ,COLOR_PAIR(1));
        mvwprintw(win, chatwin.currentline, 1, "[%s] %% ", username.c_str()); 
        wattroff(win, COLOR_PAIR(1));
        int currentchar = prefix + 1; 
        std::string input;
        int c;
        int lineoffset = 0;
        int maxchar;
        int number_of_lines = 0;
        while ((c = wgetch(win)) != '\n') { 
            maxchar = chatwinx - 3;
            wmove(upwin, 1, 1);
            wclrtoeol(upwin);
            wprintw(upwin, "%d", c);
            wmove(upwin, 1, 5);
            wprintw(upwin, "%d", chatwin.currentline);
            wrefresh(upwin);
            if (c == KEY_UP) {
                // history up
            } else if (c == KEY_DOWN) {
                // history down
            } else if (c == KEY_LEFT) {
                currentchar = std::max(prefix+1, --currentchar);
            } else if (c == KEY_RIGHT) {
                if (lineoffset == number_of_lines) {
                    currentchar = std::min(static_cast<int>(input.size()%(maxchar-prefix))+2+prefix, ++currentchar);
                } else if (currentchar == maxchar && lineoffset<number_of_lines){
                    ++lineoffset;
                    currentchar = prefix + 1;   
                } else {
                    currentchar = std::min(static_cast<int>(input.size())+1+prefix, ++currentchar);
                }
            } else if (c == KEY_RESIZE) {
                    int ch = currentchar-prefix - 1 + (lineoffset * (maxchar - prefix-1));
                    wmove(win, chatwin.currentline, 1);

                    wclrtoeol(win);
                    handle_sigwinch(SIGWINCH);
                    number_of_lines = input.length()/(maxchar-prefix-1); // 0 when on the first line
                    chatwin.upshift(number_of_lines);
                    wattron(win ,COLOR_PAIR(1));
                    mvwprintw(win, chatwin.currentline, 1, "[%s] %% ", username.c_str()); 
                    wattroff(win, COLOR_PAIR(1));
                    maxchar = chatwinx - 3;
                    currentchar = prefix + 1 + ch%(maxchar - prefix - 1);
                    lineoffset = ch/(maxchar - prefix-1);
                    draw_upwin();
            } else if (c == 127) {
                if(input.length() == 0 || currentchar == prefix + 1){
                // do nothing because there is nothing to delete
                    if(lineoffset > 0){
                        --lineoffset;
                        currentchar = maxchar;
                    }
                } else{
                input.erase(currentchar-prefix-2+lineoffset*(maxchar-prefix), 1);
                currentchar--;
                }
                } else {
                wmove(win, chatwin.currentline+lineoffset,  currentchar);
            
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
            int lines_comp = number_of_lines; // int to know if number_of_lines changed
            number_of_lines = input.length()/(maxchar-prefix-1); // 0 when on the first line
            if(chatwin.currentline + lines_comp == chatwiny-2 && number_of_lines > lines_comp) {
                f++;
                wmove(win, chatwin.currentline, 1);
                wclrtoeol(win);
                chatwin.upshift(number_of_lines);
                wattron(win ,COLOR_PAIR(1));
                mvwprintw(win, chatwin.currentline, 1, "[%s] %% ", username.c_str()); 
                wattroff(win, COLOR_PAIR(1));
            }
        std::vector<int> tmp; 
        for (int i = 0; i <= number_of_lines; ++i){
            wmove(win, chatwin.currentline + i, prefix + 1);
            wclrtoeol(win);
            int startidx = i * (maxchar - prefix - 1);
            int endidx = std::min(startidx + maxchar-prefix - 1 , static_cast<int>(input.length()));
            try{
            wprintw(win, "%s", input.substr(startidx, endidx-startidx).c_str());
                } catch(std::exception const& ec){
                    std::cerr << "substringing" << ec.what() << std::endl;
                }
        }
        for(int j = 0; j < tmp.size(); ++j){
            wmove(upwin, 1, 10+(j*5));
            wprintw(upwin, "%d", tmp[j]);
        }
        wrefresh(upwin);
        draw_chat();
        wmove(win, chatwin.currentline+lineoffset,  currentchar);
        wrefresh(win);
        }
        wrefresh(win); 
        wmove(win, chatwin.currentline, 0);
        wclrtoeol(win);
        if(input.length()>0){
        chatwin.writeline(input, username, 1);
        }
        werase(win);
        chatwin.print();
       
        wmove(upwin, 1, 5);
        wprintw(upwin, "%d", chatwin.currentline);
        wrefresh(upwin);
        draw_chat();
        wmove(win, chatwin.currentline, prefix + input.length());
        wrefresh(win);
    }

    delwin(win);
    delwin(upwin);
    endwin();
    return 0;
}


void read(boost::beast::flat_buffer& buffer, boost::beast::websocket::stream<tcp::socket>& ws, ChatWindow* chatwin_){

        ws.async_read(buffer, [&ws, &buffer](boost::beast::error_code ec, std::size_t byte_transfered){
            if(ec == boost::beast::websocket::error::closed) { 
                ws.close(boost::beast::websocket::close_code::normal);
                return;
                }
            if(ec) {
                std::cerr << "read Error in wsClient: " << ec << ec.message() << std::endl;
                return;
            }
            std::string output = boost::beast::buffers_to_string(buffer.data());
            chatwin_.writeline();

            buffer.consume(buffer.size());

            read(buffer, ws);
        
        });

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

    // tcp connection
    boost::asio::connect(ws.next_layer(), results);

    ws.handshake(argv[2], "/");

    boost::beast::flat_buffer buffer;

    ChatWindow chatwin(win);

    std::thread UI(UI_thread(chatwin));
    
    ioc.run();

    }
    catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}