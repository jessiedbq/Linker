//
//  linker.cpp
//  Linker
//
//  Created by Jessie on 15/02/2018.
//  Copyright © 2018 Jessie. All rights reserved.
//


#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <istream>
#include <locale>
#include <string>
#include <map>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <algorithm>

using namespace std;

map<string, int>symbol_count;
map<string, int> sym_addr;
map<string, int>symbol_deftimes;
map<string, int> symbol_modcount;//symbol and modulecount pair;

vector<int> offsetList;
vector<int> lengthList; //the length of each sentence
vector<int> lineidList;
vector<string>wordList;
vector<int> moduleAddr;//module address
vector<string>usedList;//symbol used in instruction in pass two


int i = 0;  //index

struct Deflist{
    int defCount;
    vector<string> symbolList;
    vector<int> symbolAddrList;
};

struct Uselist{
    int useCount;
    vector<string> useList;
};

struct Instlist{
    int codeCount;
    vector<string> typeList;
    vector<int> addrList;
};

void parse_error (int errcode, int linenum, int lineoffset)
{
    static const char* errstr[] =   {
        "NUM_EXPECTED",           //0: Number expected
        "SYM_EXPECTED",           //1: Symbol expected
        "ADDR_EXPECTED",          //2: Addressing Expected which is A/E/I/R
        "SYM_TOO_LONG",           //3: Symbol Name is too long
        "TOO_MANY_DEF_IN_MODULE", //4: > 16
        "TOO_MANY_USE_IN_MODULE", //5: > 16
        "TOO_MANY_INSTR",         //6: Total num_instr exceeds memory size (512)
    };
    printf("Parse Error line %d offset %d: %s\n", linenum, lineoffset, errstr[errcode]);
}

void readFile(string filename){
    ifstream infile;
    infile.open(filename.c_str());
    
    if (!infile.is_open()){
        cout << "Open Fails!" << endl;
    }
    else{
        int lineID = 0;
        string line;
        string word;
        stringstream iss;
        while(getline(infile, line)){
            iss.clear();
            iss << line;
            lengthList.push_back(line.length());
            lineID++;
            string token;
            int line_offset = 0;
            while(iss >> token){
                //find the position where token appears first
                line_offset = line.find(token, line_offset);
                lineidList.push_back(lineID);
                offsetList.push_back(line_offset+1);
                wordList.push_back(token);
            }
        }
        lineidList.push_back(lineID);
        offsetList.push_back(lengthList.back()+1);
    }
    infile.close();
}

//check number form
bool isNumber(int i){
    if(i==wordList.size()){
        return false;
    }
    for(int k = 0;k<wordList[i].size();k++)
    {
        if(!isdigit(wordList[i][k]))
        return false;
    }
    return true;

}


//check the symbol form
bool isSymbol(int i) {
    if(i==wordList.size()){
        return false;
    }
    string sym = wordList[i];
    char c = sym[0];
    if(!isalpha(c)){
        return false;
    }
        for( int j = 1; j < sym.size(); j++ ){
            if( !isalnum(sym[j])) {
                return false;
            }
        }
        return true;
    }

//check IARE type form
bool isType(int i){
    if(i==wordList.size()){
        return false;
    }
    string type = wordList[i];
    if( type =="I"||type =="A"||type =="E"||type =="R"){
        return true;
    }
    else
    return false;
}



bool readDefList(vector<Deflist>& dList){
    if (!isNumber(i)){
        parse_error(0, lineidList[i], offsetList[i]);
        return false;
    }
    
    int defcount = atoi(wordList[i].c_str());
    if(defcount> 16){
        parse_error(4,lineidList[i], offsetList[i]);
        return false;
    }
    
    Deflist dlist;
    dlist.defCount= defcount;
    
    for (int j=0; j < defcount; j++){
        i++;
      
        if(!isSymbol(i)){
            parse_error(1, lineidList[i], offsetList[i]);
            return false;
        }
        
        if (wordList[i].length()>16){
            parse_error(3, lineidList[i], offsetList[i]);
            return false;
        }
        
        string token=wordList[i];
        dlist.symbolList.push_back(token);
        
        if(symbol_modcount.find(token)==symbol_modcount.end()){
            symbol_modcount.insert(make_pair(token,dList.size()));
            
        }
        
        symbol_deftimes[token]+=1;
        i++;
   
        if(!isNumber(i)){
            parse_error(0, lineidList[i], offsetList[i]);
            return false;
        }
        dlist.symbolAddrList.push_back(atoi(wordList[i].c_str()));
    }
    dList.push_back(dlist);
    return true;
}

bool readUseList(vector<Uselist>& uList){
    if(i==wordList.size()){
        return false;
    }
    Uselist ulist;
    i++;
 
    if(!isNumber(i)){
        parse_error(0, lineidList[i], offsetList[i]);
        return false;
    }
    
    ulist.useCount = atoi(wordList[i].c_str());
    if( ulist.useCount>16){
        parse_error(5, lineidList[i], offsetList[i]);
        return false;
    }
    
    for(int j=0; j<ulist.useCount;j++){
        i++;
    
        if(!isSymbol(i)){
            parse_error(1, lineidList[i], offsetList[i]);
            return  false;
        }
        if(wordList[i].length()> 16){
            parse_error(3, lineidList[i], offsetList[i]);
            return false;
        }
        ulist.useList.push_back(wordList[i]);
    }
     uList.push_back(ulist);
    return true;
}


bool readInstList(vector<Instlist>& iList, int& modAddr){
    if(i==wordList.size()){
        return false;
    }
    i++;
    Instlist ilist;
 
    if(!isNumber(i)){
      parse_error(0, lineidList[i], offsetList[i]);
        return false;
    }
      ilist.codeCount =atoi(wordList[i].c_str());
      if(ilist.codeCount > (512 -modAddr)){
          parse_error(6, lineidList[i], offsetList[i]);
          return false;
      }
      modAddr +=ilist.codeCount;
    
      for(int j=0;j<ilist.codeCount;j++){
          i++;
     
          if(!isType(i))
          {
               parse_error(2, lineidList[i], offsetList[i]);
              return false;
              
          }
          ilist.typeList.push_back(wordList[i]);
          i++;
     
          if(!isNumber(i)){
              parse_error(0, lineidList[i], offsetList[i]);
              return false;
          }
          //string to int;
          int instr = atoi(wordList[i].c_str());
          ilist.addrList.push_back(instr);
      }
    iList.push_back(ilist);
    return true;
}


int main(int argc, const char * argv[]) {
   
    if(argc != 2){
        cout<<"Please enter the input file again!"<<endl;
        exit(0);
    }
    
    vector<Deflist> dList; //definition list
    vector<Uselist> uList; //use list
    vector<Instlist> iList; //instruction list
    int modcount = 0;//module count
    int modAddr = 0;//module address
    moduleAddr.push_back(modAddr);
    readFile(argv[1]);
    
    //Pass one
    while(i<wordList.size()){
        if (!readDefList(dList)){
            return 1;
        }
        
        if (!readUseList(uList)){
            return 1;
        }
        
        if (!readInstList(iList,modAddr)){
            return 1;
        }
        i++;
        moduleAddr.push_back(modAddr);
    }
    
    vector<string> defList_A;
    vector<string> symbolList_A;
    for(int j=0; j < dList.size() ; j ++){
        for (int k =0; k <dList[j].defCount;k++){
            string symbol = dList[j].symbolList[k];
            int sym_addr = dList[j].symbolAddrList[k];
            if(find(defList_A.begin(),defList_A.end(), symbol)==defList_A.end()){
                if(sym_addr >= iList[j].codeCount){
                    cout << "Warning: Module " << j+1 << ": "<< symbol << " too big " << sym_addr
                    << " (max=" << (iList[j].codeCount-1)<< ") assume zero relative" << "\n" << endl;
                    dList[j].symbolAddrList[k]=0;
                }
                defList_A.push_back(symbol);
            }
        }
    }
    
    cout << "Symbol Table\n";
    for(int j=0;j<dList.size();j++){
        if(dList[j].defCount!=0){
            for(int k = 0; k<dList[j].symbolList.size();k++){
                string sym = dList[j].symbolList[k];
                int sym_address = moduleAddr[j]+dList[j].symbolAddrList[k];
                sym_addr.insert(make_pair(sym, sym_address));
                //if don't find the symbol
                if(find(symbolList_A.begin(),symbolList_A.end(), sym)==symbolList_A.end()){
                    cout<<sym<<"="<<sym_address;
                    if (symbol_deftimes[sym]>1){
                        cout<<" Error: This variable is multiple times defined; first value used";
                    }
                    cout<<endl;
                    symbolList_A.push_back(sym);
                }
            }
        }
    }
    cout<<endl;
    
    
    //Pass two
    int baseAddr = 0;
    vector<int>baseAddrList;
    baseAddrList.push_back(baseAddr);
    
    for (int k=0; k<iList.size(); k++) {
        baseAddr+=iList[k].codeCount;
        baseAddrList.push_back(baseAddr);
    }
    
    cout << "Memory Map\n";
    while(modcount<dList.size()){
        
        for(int j = 0; j<iList[modcount].codeCount;j++){
            string type = iList[modcount].typeList[j];
            int addr = iList[modcount].addrList[j];
            int final_oprand;  //the converted oprand
            int opcode = addr / 1000;
            int oprand = addr % 1000;
            string err_message;
            
            
            //Check the instruction type and convert the address.
            if( type == "I" ) {
                if ( (opcode*1000 + oprand) >= 10000){
                    err_message = "Error: Illegal immediate value; treated as 9999";
                    final_oprand = 999;
                    opcode = 9;
                }
                else {
                    final_oprand = oprand;
                }
            }
            else if (type == "A") {
                if (opcode >= 10) {
                    opcode = 9;
                    final_oprand = 999;
                    err_message = "Error: Illegal opcode; treated as 9999";
                }
                else{
                    final_oprand = oprand;
                    if( final_oprand > 511 ) {
                        err_message = "Error: Absolute address exceeds machine size; zero used";
                        final_oprand = 0;
                    }
                }
            }
            else if (type == "R") {
                if (opcode >= 10) {
                    opcode = 9;
                    final_oprand=999;
                    err_message = "Error: Illegal opcode; treated as 9999";
                }
                else{
                    if( oprand > iList[modcount].codeCount) {
                        err_message = "Error: Relative address exceeds module size; zero used";
                        oprand = 0;
                    }
                    final_oprand = baseAddrList[modcount] + oprand;
                }
                
            }
            else if (type == "E") {
                if (opcode >= 10) {
                    opcode = 9;
                    final_oprand=999;
                    err_message = "Error: Illegal opcode; treated as 9999";
                }
                else{
                    if( oprand >= uList[modcount].useList.size() ) {
                        err_message = "Error: External address exceeds length of uselist; treated as immediate";
                        final_oprand = oprand;
                        
                    }
                    else {
                        // Relate the address to the external symbol.
                        bool defined = false;
                        string sym = uList[modcount].useList[oprand];
                        for(int j = 0; j< dList.size();j++){
                            if(find(dList[j].symbolList.begin(),dList[j].symbolList.end(),sym) != dList[j].symbolList.end() ) {
                                defined = true;
                                
                            }
                        }
                        if(!defined){
                            err_message = "Error: " + sym + " is not defined; zero used";
                            final_oprand = 0;
                        }
                        else {
                            final_oprand = sym_addr[sym];
                        }
                        usedList.push_back(sym);
                        
                    }
                }
            }
            
            cout<< setw(3)<<setfill('0')<<(baseAddrList[modcount]+j)<< ": "<<setw(4)<<setfill('0')<< (final_oprand + 1000 * opcode);
            if( err_message.size() > 0 )
            cout<<" "<< err_message<<endl;
            else
            cout<<endl;
            }
        
        for( int j = 0; j < uList[modcount].useCount ;j++ ) {
            
        string sym =  uList[modcount].useList[j];
            if(find(usedList.begin(), usedList.end(),sym)==usedList.end())
            cout << "Warning: Module " << modcount+1 <<": "
        << sym
        << " appeared in the uselist but was not actually used"
        << endl;
        }
         modcount++;
        
    }
    vector<string> defSymbolList;
    for(int j=0;j<dList.size();j++){
        for(int k= 0;k<dList[j].defCount;k++){
            string symbol = dList[j].symbolList[k];
            if(find(defSymbolList.begin(),defSymbolList.end(),symbol)==defSymbolList.end()){
                defSymbolList.push_back(symbol);
            }
        }
    }
    
    cout<<"\n";
    
    for(int j = 0; j<defSymbolList.size();j++){
        string sym = defSymbolList[j];
            if(find(usedList.begin(),usedList.end(),sym)==usedList.end()){
             cout <<"Warning: Module "<<symbol_modcount[sym]+1<<": "<<
              sym << " was defined but never used" << endl;
        }
    }
    
    cout<<"\n";
    
    return 0;

}
        

    




            
            

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
