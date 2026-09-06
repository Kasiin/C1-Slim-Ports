#pragma once
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../assets/puzzles.h"

struct Game {
    int level=0, number=0, cursor=0, hints=0;
    bool active=false;
    std::array<int,3> next{{0,0,0}};
    std::array<int,81> given{}, board{}, solution{};
    struct Edit { int cell, previous; };
    std::vector<Edit> history;
    void start(int l,int n) {
        level=l;number=n;cursor=0;hints=0;active=true;history.clear();
        next[l]=(n+1)%200;
        for(int i=0;i<81;i++) {
            given[i]=board[i]=PUZZLES[l][n][0][i]-'0';
            solution[i]=PUZZLES[l][n][1][i]-'0';
        }
    }
    bool conflicts(int cell,int digit) const {
        if(!digit)return false;
        for(int i=0;i<81;i++)
            if(i!=cell && board[i]==digit &&
               (i/9==cell/9 || i%9==cell%9 || (i/27==cell/27 && i%9/3==cell%9/3)))return true;
        return false;
    }
    bool put(int digit) {
        if(!active || given[cursor] || digit<0 || digit>9 || conflicts(cursor,digit))return false;
        if(board[cursor]==digit)return true;
        history.push_back({cursor,board[cursor]});board[cursor]=digit;return true;
    }
    bool undo() {
        if(history.empty())return false;
        auto e=history.back();history.pop_back();cursor=e.cell;board[cursor]=e.previous;return true;
    }
    int hint() {
        if(!active || won())return false;
        int target=-1;
        // Repair an earlier wrong entry first so a revealed answer cannot
        // silently conflict with player input elsewhere in the grid.
        for(int i=0;i<81;i++)if(board[i] && board[i]!=solution[i]){target=i;break;}
        if(target<0 && !given[cursor] && !board[cursor])target=cursor;
        if(target<0)for(int i=0;i<81;i++)if(!board[i]){target=i;break;}
        if(target<0)return false;
        bool wrong=board[target]!=0 && board[target]!=solution[target];
        history.push_back({target,board[target]});cursor=target;
        // Clear wrong input first, avoiding a new duplicate against a second
        // incorrect player entry. A later hint can safely reveal the answer.
        board[target]=wrong?0:solution[target];hints++;return wrong?2:1;
    }
    bool won() const { return active && board==solution; }
    int filled() const {return std::count_if(board.begin(),board.end(),[](int n){return n!=0;});}
    std::string candidates() const {
        std::string s;
        for(int d=1;d<=9;d++)if(!conflicts(cursor,d))s+=char('0'+d);
        return s.empty()?"-":s;
    }
    bool save(const std::string &path) const {
        std::string tmp=path+".new";
        FILE*f=std::fopen(tmp.c_str(),"wb");if(!f)return false;
        bool ok=std::fprintf(f,"C1SUDOKU1 %d %d %d %d %d %d %d %d\n",active,level,number,cursor,hints,next[0],next[1],next[2])>0;
        for(int v:board)ok=std::fputc('0'+v,f)!=EOF && ok;
        ok=std::fputc('\n',f)!=EOF && ok;
        if(std::fclose(f))ok=false;
        if(ok && std::rename(tmp.c_str(),path.c_str())==0)return true;
        std::remove(tmp.c_str());return false;
    }
    bool load(const std::string &path) {
        FILE*f=std::fopen(path.c_str(),"rb");if(!f)return false;
        int a,l,n,c,h,x,y,z;char magic[32]{}, digits[90]{};
        bool ok=std::fscanf(f,"%31s %d %d %d %d %d %d %d %d %89s",magic,&a,&l,&n,&c,&h,&x,&y,&z,digits)==10;
        std::fclose(f);
        if(!ok || std::strcmp(magic,"C1SUDOKU1") || a!=1 || l<0 || l>2 || n<0 || n>=200 || c<0 || c>80 || h<0 || h>100000 || x<0 || x>=200 || y<0 || y>=200 || z<0 || z>=200 || std::strlen(digits)!=81)return false;
        Game candidate;candidate.start(l,n);candidate.cursor=c;candidate.hints=h;candidate.next={{x,y,z}};
        for(int i=0;i<81;i++){
            int d=digits[i]-'0';
            if(d<0 || d>9 || (candidate.given[i] && d!=candidate.given[i]))return false;
            candidate.board[i]=d;
        }
        for(int i=0;i<81;i++)if(candidate.conflicts(i,candidate.board[i]))return false;
        *this=candidate;return true;
    }
};
