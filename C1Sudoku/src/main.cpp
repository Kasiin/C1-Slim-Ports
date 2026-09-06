#include "game.h"
#include "../assets/font.h"
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>
#include <chrono>

static volatile sig_atomic_t stopped=0;
static void stop_handler(int){stopped=1;}
static uint64_t milliseconds(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
static const char* levels[]={"简单","普通","困难"};
static const char* SAVE="/storage/c1sudoku/save.txt";
using Frame=std::array<uint8_t,5624>;
static Frame frame{};
static void pixel(int x,int y,bool black=true){if(x>=0&&x<296&&y>=0&&y<152){auto &b=frame[(y/8)*296+x];unsigned m=0x80>>(y%8);if(black)b|=m;else b&=~m;}}
static void rect(int x,int y,int w,int h,bool black=true){for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)pixel(i,j,black);}
static uint32_t rune(const char*&s){uint32_t c=(unsigned char)*s++;if(c<128)return c;int n=c<224?1:c<240?2:3;c&=(1<<(6-n))-1;while(n--&&*s)c=(c<<6)|((unsigned char)*s++&63);return c;}
static void text(int x,int y,const std::string &str,bool black=true,bool bold=false){const char*s=str.c_str();while(*s){uint32_t cp=rune(s);const Glyph*g=nullptr;for(auto &entry:FONT)if(entry.code==cp){g=&entry;break;}if(!g){x+=8;continue;}for(int row=0;row<16;row++)for(int col=0;col<g->width;col++)if(g->rows[row]&(0x8000>>col)){pixel(x+col,y+row,black);if(bold)pixel(x+col+1,y+row,black);}x+=g->width;}}
static std::string fmt(const char *format,int a,int b=0){char s[80];std::snprintf(s,sizeof(s),format,a,b);return s;}

struct UI {
    Game game;
    bool menu=true,confirm=false;
    int selected=0,number=0;
    std::string message;
    std::string savePath=SAVE;
    bool dirty=true;
    void render(){
        frame.fill(0);
        if(menu){
            text(8,2,"数独  SUDOKU",true,true);
            text(178,2,"内置600题");rect(8,23,280,1);
            for(int l=0;l<3;l++){
                int y=32+l*25;
                if(selected==l)rect(8,y,280,23);
                text(15,y+3,std::string(l==0?"Q ":l==1?"W ":"E ")+levels[l],selected!=l);
                text(115,y+3,fmt("%03d / 200",selected==l?number+1:game.next[l]+1),selected!=l);
                text(216,y+3,l==0?"入门":l==1?"进阶":"挑战",selected!=l);
            }
            if(confirm){
                rect(8,110,280,40);text(15,110,"新题会替换当前进度",false);
                text(15,130,"Enter确认  Back取消",false);
            }else{
                text(8,112,"上下选难度 左右选题号");
                text(8,132,game.active?"Enter开始 C继续 Back退出":"Enter开始 Back退出");
            }
            return;
        }
        // 16-pixel cells, 2-pixel box separators, absolutely no greyscale.
        for(int i=0;i<=9;i++){
            rect(i*16,3,i%3==0?2:1,146);rect(0,3+i*16,146,i%3==0?2:1);
        }
        int cell=game.cursor,x=(cell%9)*16+2,y=3+(cell/9)*16+2;
        rect(x,y,13,13);
        for(int i=0;i<81;i++)if(game.board[i]){
            int xx=i%9*16+4,yy=3+i/9*16;
            text(xx,yy,std::string(1,'0'+game.board[i]),i!=cell,game.given[i]!=0);
            if(!game.given[i])rect(xx,yy+14,7,1,i!=cell);
        }
        text(153,0,std::string("数独 ")+levels[game.level],true,true);
        text(153,16,fmt("%03d  %02d/81",game.number+1,game.filled()));
        text(153,32,"方向键 移动");
        text(153,48,"Q~O 填入1~9");
        text(153,64,"H提示 DEL擦除");
        text(153,80,"Z撤销 N选题");
        text(153,96,"Back保存退出");
        if(game.won()){
            text(153,112,"完成! N选新题",true,true);
            text(153,132,fmt("提示使用 %d 次",game.hints));
        }else{
            text(153,112,fmt("行%d 列%d",cell/9+1,cell%9+1)+(game.given[cell]?" 已知":""));
            text(153,132,message.empty()?"候选:"+game.candidates():message);
        }
    }
    bool persist(){if(!game.active)return true;bool ok=game.save(savePath);if(!ok)message="存档失败!";return ok;}
    bool key(int code){
        dirty=true;
        if(code==KEY_HOME)return false; // HOME is reserved for Terminal only.
        if(menu){
            if(confirm){
                if(code==KEY_ENTER||code==KEY_OK){game.start(selected,number);menu=false;confirm=false;message.clear();persist();}
                else if(code==KEY_BACK||code==KEY_ESC)confirm=false;
                return false;
            }
            if(code==KEY_BACK||code==KEY_ESC)return true;
            if(code==KEY_C&&game.active){menu=false;return false;}
            if(code==KEY_UP)selected=(selected+2)%3;
            else if(code==KEY_DOWN)selected=(selected+1)%3;
            else if(code==KEY_Q)selected=0;
            else if(code==KEY_W)selected=1;
            else if(code==KEY_E)selected=2;
            else if(code==KEY_LEFT){number=(number+199)%200;return false;}
            else if(code==KEY_RIGHT){number=(number+1)%200;return false;}
            else if(code==KEY_ENTER||code==KEY_OK){
                if(game.active&&!game.won()){confirm=true;return false;}
                game.start(selected,number);menu=false;message.clear();persist();return false;
            }else return false;
            number=game.next[selected];return false;
        }
        message.clear();
        if(code==KEY_BACK||code==KEY_ESC){persist();return true;}
        if(code==KEY_N){persist();menu=true;selected=game.level;number=game.next[selected];return false;}
        if(code==KEY_LEFT)game.cursor=game.cursor/9*9+(game.cursor%9+8)%9;
        else if(code==KEY_RIGHT)game.cursor=game.cursor/9*9+(game.cursor%9+1)%9;
        else if(code==KEY_UP)game.cursor=(game.cursor+72)%81;
        else if(code==KEY_DOWN)game.cursor=(game.cursor+9)%81;
        else if(code==KEY_H){int result=game.hint();if(result){message=result==2?"提示:清除错数":"提示已填入";persist();}}
        else if(code==KEY_Z){if(!game.undo())message="没有可撤销项";else persist();}
        else{
            int digit=-1;
            if(code>=KEY_Q&&code<=KEY_O)digit=code-KEY_Q+1;
            if(code>=KEY_1&&code<=KEY_9)digit=code-KEY_1+1;
            if(code==KEY_DELETE||code==KEY_BACKSPACE||code==KEY_P||code==KEY_0)digit=0;
            if(digit>=0){
                if(!game.put(digit))message=game.given[game.cursor]?"已知数字不可改":"行列宫内重复!";
                else persist();
            }
        }
        return false;
    }
};

struct Inputs {
    std::vector<pollfd> fds;
    std::array<bool,KEY_MAX+1> down{};
    Inputs(){try{for(auto path:{"/dev/input/event0","/dev/input/event1"}){
        int fd=open(path,O_RDONLY|O_NONBLOCK|O_CLOEXEC);
        if(fd<0)throw std::runtime_error("open input failed");
        if(ioctl(fd,EVIOCGRAB,1)<0){close(fd);throw std::runtime_error("exclusive input grab failed");}
        fds.push_back({fd,POLLIN,0});
    }}catch(...){for(auto p:fds)close(p.fd);throw;}}
    ~Inputs(){for(auto p:fds)close(p.fd);}
    template<class F> void poll_events(F callback,int timeout){
        if(poll(fds.data(),fds.size(),timeout)<=0)return;
        for(auto p:fds)if(p.revents&POLLIN){
            input_event ev;
            while(read(p.fd,&ev,sizeof(ev))==sizeof(ev))if(ev.type==EV_KEY && ev.code<=KEY_MAX){
                down[ev.code]=ev.value!=0;callback(ev.code,ev.value);
            }
        }
    }
    void drain(){
        uint64_t quiet=0;
        while(!stopped){
            poll_events([](int,int){},20);
            if(std::any_of(down.begin(),down.end(),[](bool b){return b;}))quiet=0;
            else if(!quiet)quiet=milliseconds();
            else if(milliseconds()-quiet>=150)return;
        }
    }
};

static void write_frame(){
    int fd=open("/dev/epaper_lcd",O_WRONLY|O_NONBLOCK|O_CLOEXEC);
    if(fd<0)throw std::runtime_error("display open failed");
    auto n=write(fd,frame.data(),frame.size());close(fd);
    if(n!=(ssize_t)frame.size())throw std::runtime_error("display write failed");
}
static bool dump(const char*path){FILE*f=fopen(path,"wb");if(!f)return false;bool ok=fwrite(frame.data(),1,frame.size(),f)==frame.size();if(fclose(f))ok=false;return ok;}
static int selftest(){
    Game g;g.start(0,0);
    for(int i=0;i<81;i++)if(g.given[i]){g.cursor=i;if(g.put(0))return 1;break;}
    for(int i=0;i<81;i++)if(!g.given[i]){g.cursor=i;break;}
    int i=g.cursor;if(!g.put(g.solution[i])||!g.undo()||g.board[i])return 2;
    if(!g.hint()||g.hints!=1||g.board[i]!=g.solution[i])return 3;
    if(!g.save("/tmp/c1sudoku-selftest.txt"))return 4;
    Game loaded;if(!loaded.load("/tmp/c1sudoku-selftest.txt")||loaded.board!=g.board)return 5;
    FILE*bad=fopen("/tmp/c1sudoku-selftest.txt","wb");if(!bad)return 8;
    fputs("C1SUDOKU1 1 0 999 0 0 0 0 0\n000\n",bad);fclose(bad);
    auto old=loaded.board;if(loaded.load("/tmp/c1sudoku-selftest.txt")||loaded.board!=old)return 9;
    remove("/tmp/c1sudoku-selftest.txt");
    UI ui;ui.savePath="/tmp/c1sudoku-ui-test.txt";ui.game.start(0,0);ui.menu=false;
    for(int d=1;d<=9;d++){
        int cell=-1;
        for(int k=0;k<81;k++)if(!ui.game.given[k]&&ui.game.solution[k]==d){cell=k;break;}
        if(cell<0)continue;
        ui.game.cursor=cell;
        ui.key(KEY_Q+d-1);
        if(ui.game.board[cell]!=d)return 10;
        ui.key(KEY_Z);if(ui.game.board[cell])return 11;
    }
    bool wrongTested=false;
    for(int k=0;k<81&&!wrongTested;k++)if(!ui.game.given[k])for(int d=1;d<=9;d++){
        if(d==ui.game.solution[k]||ui.game.conflicts(k,d))continue;
        ui.game.cursor=k;ui.game.put(d);
        if(ui.game.hint()!=2||ui.game.board[k]!=0)return 12;
        wrongTested=true;break;
    }
    if(ui.key(KEY_HOME))return 13;
    ui.key(KEY_N);if(!ui.menu)return 14;
    ui.key(KEY_W);if(ui.selected!=1)return 15;
    ui.key(KEY_ENTER);if(!ui.confirm)return 16;
    ui.key(KEY_BACK);if(ui.confirm||!ui.menu)return 17;
    ui.key(KEY_C);if(ui.menu)return 18;
    if(!ui.key(KEY_BACK))return 19;
    remove("/tmp/c1sudoku-ui-test.txt");
    for(int l=0;l<3;l++)for(int n=0;n<200;n++){
        g.start(l,n);int count=0;while(g.hint())if(++count>81)return 6;
        if(!g.won())return 7;
    }
    puts("PASS: givens, all Q-O digits, Z undo, hint repair, save/load, corrupt saves, menus, HOME isolation, all 600 completions");return 0;
}
int main(int argc,char**argv){try{
    if(argc==2&&std::string(argv[1])=="--version"){puts("C1Sudoku 1.0.0 / 600 puzzles / 1-bit");return 0;}
    if(argc==2&&std::string(argv[1])=="--selftest")return selftest();
    UI ui;
    if(argc==3){
        if(std::string(argv[1])=="--preview-game"){ui.game.start(0,0);ui.menu=false;}
        else if(std::string(argv[1])!="--preview-menu")return 2;
        ui.render();return dump(argv[2])?0:1;
    }
    signal(SIGINT,stop_handler);signal(SIGTERM,stop_handler);signal(SIGHUP,stop_handler);
    ui.game.load(SAVE);if(ui.game.active){ui.selected=ui.game.level;ui.number=ui.game.next[ui.selected];}
    Inputs inputs;
    // Swallow launch-key tail events before accepting menu shortcuts.
    uint64_t start=milliseconds();while(milliseconds()-start<180)inputs.poll_events([](int,int){},20);
    Frame previous{};bool valid=false,exit=false;uint64_t lastWrite=0,lastRepeat=0;
    while(!stopped&&!exit){
        inputs.poll_events([&](int code,int value){
            if(exit)return;
            bool arrow=code==KEY_UP||code==KEY_DOWN||code==KEY_LEFT||code==KEY_RIGHT;
            if(value==1||(value==2&&arrow&&milliseconds()-lastRepeat>=150)){
                lastRepeat=milliseconds();exit=ui.key(code);
            }
        },30);
        if(!exit&&ui.dirty&&milliseconds()-lastWrite>=90){
            ui.render();if(!valid||frame!=previous){write_frame();previous=frame;valid=true;lastWrite=milliseconds();}
            ui.dirty=false;
        }
    }
    ui.persist();inputs.drain();return 0;
}catch(const std::exception&e){fprintf(stderr,"c1sudoku: %s\n",e.what());return 1;}}
