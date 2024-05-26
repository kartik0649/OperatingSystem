#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <map>
#include <vector>
#include <string>
#include <cstdlib>  // For exit()
#include <cctype>  // For character checking functions

// #define MACHINE_SIZE (512)
// #define OPCODE_SIZE (10)
// #define DEF_SIZE (16)
// #define OPERAND_I (900)
// #define FOUR_NINE (9999)
// #define OPCODE_MAX (9)


class Token {
public:
    std::string value;  // The string value of the token.
    int lineNumber;     // Line number where the token appears.
    int lineOffset;     // Character offset within the line.

    Token(const std::string& val, int lineNum, int lineOff)
        : value(val), lineNumber(lineNum), lineOffset(lineOff) {}
};

class Symbol {
public:
    std::string name;         // Name of the symbol
    int moduleAddress;        // Address of the module where it is defined
    int relativeAddress;      // Relative address within the module
    int moduleNumber;         // Which module number this symbol belongs to
    bool symbolAlreadyDefined; // Flag to track if symbol is defined multiple times
    bool isInUseList;         // Flag to track if symbol is used in any use list
    bool isInDefList;           //Flag to track if symbol is in Deflist for error checking
    bool definedAndUsed;             // Flag to check if the symbol was actually used in instructions

    Symbol(const std::string& symbolName, int modAddr, int relAddr, int modNum)
        : name(symbolName), moduleAddress(modAddr), relativeAddress(relAddr),
          moduleNumber(modNum), symbolAlreadyDefined(false), isInUseList(false), isInDefList(false), definedAndUsed(false) {}
};

std::vector<Symbol> symbolTable;

class Module {
public:
    int baseAddress;           // Base address for relative addressing
    int definitionCount;       // Number of definitions in the module
    int useCount;              // Number of uses in the module
    int instructionCount;      // Number of instructions in the module

    Module(int base = 0, int defCount = 0, int useC = 0, int instrCount = 0)
        : baseAddress(base), definitionCount(defCount), useCount(useC), instructionCount(instrCount) {}
};

std::vector<Module> moduleBaseTable;

Token* getToken(std::ifstream &inputFile, int &lineNumber, int &offsetNumber, int &lineLength, char *&linePtr) {
    char *tokenPtr = nullptr;
    std::string line;

    // Check if this is the first call to getToken by checking if linePtr is nullptr.
    if (linePtr == nullptr || *linePtr == '\0') {  // Ensuring linePtr is either not initialized or pointing to an empty string.
        if (getline(inputFile, line)) {
            lineNumber++;  // Increment line number as we read a new line.
            lineLength = line.length();
            delete[] linePtr;  // Clean up previous buffer if any.
            linePtr = new char[line.length() + 1];  // Allocate memory for the new line.
            strcpy(linePtr, line.c_str());  // Copy line content into buffer.
            tokenPtr = strtok(linePtr, " \t");  // Begin tokenization on the new buffer.
        }
    } else {
        tokenPtr = strtok(nullptr, " \t");  // Continue tokenization of the existing buffer.
    }

    // If no token was found in the initial buffer or after continuing, attempt to read next lines.
    while (tokenPtr == nullptr && getline(inputFile, line)) {
        lineNumber++;  // Increment line number with each new line read.
        lineLength = line.length();  // Update the length of the current line.

        delete[] linePtr;  // Free the previous buffer.
        linePtr = new char[line.length() + 1];  // Allocate new buffer.
        strcpy(linePtr, line.c_str());  // Copy the new line into the buffer.

        tokenPtr = strtok(linePtr, " \t");  // Start tokenization of the new line.
    }

    if (!tokenPtr) {
        if (linePtr != nullptr) {
            offsetNumber = lineLength + 1;  // Set offset to end if no tokens are found.
        }
        return nullptr;  // Return nullptr if no more tokens are available.
    }

    offsetNumber = static_cast<int>(tokenPtr - linePtr) + 1;  // Calculate the offset of the token.

    // Print the token details before returning.
    // std::cout << "Processing Token: '" << tokenPtr
    //      << "' at Line: " << lineNumber
    //      << ", Offset: " << offsetNumber << std::endl;

    // Return new token with details.
    return new Token(tokenPtr, lineNumber, offsetNumber);
}

int readInt(std::ifstream &file, int &linenum, int &offsetnum, int &linelength, char* &lineCStr) {
    Token *numericToken = getToken(file, linenum, offsetnum, linelength, lineCStr);
    if (numericToken == nullptr) {
        return 0; // Indicates no token was available, possibly end of file or read error
    }

    // Verify that the token is a valid integer
    for (char digit : numericToken->value) {
        if (!std::isdigit(digit)) {
            const char* errorMessage = "NUM_EXPECTED"; // Error message for non-digit characters
            printf("Parse Error line %d offset %d: %s\n", linenum, offsetnum, errorMessage);
            exit(0);
        }
    }

    // Convert the valid integer string to an integer value and return
    return std::stoi(numericToken->value);
}

Token* readSymbol(std::ifstream &file, int &linenum, int &offsetnum, int &linelength, char *&lineCStr) {
    Token *tokenSymbol = getToken(file, linenum, offsetnum, linelength, lineCStr);
    if (!tokenSymbol) {
        std::cout << "Parse Error line " << linenum << " offset " << offsetnum << ": SYM_EXPECTED\n";
        exit(0);
    }

    const std::string &sym = tokenSymbol->value;
    if (sym.empty() || !isalpha(sym[0])) {
        std::cout << "Parse Error line " << linenum << " offset " << offsetnum << ": SYM_EXPECTED\n";
        exit(0);
        
    }

    for (char ch : sym) {
        if (!isalnum(ch) && ch != '_') {
            std::cout << "Parse Error line " << linenum << " offset " << offsetnum << ": SYM_EXPECTED\n";
            exit(0);
            
        }
    }

    if (sym.length() > 16) {
        std::cout << "Parse Error line " << linenum << " offset " << offsetnum << ": SYM_TOO_LONG\n";
        exit(0);
    }

    return tokenSymbol;
}

Token* readMARIE(std::ifstream &file, int &linenum, int &offsetnum, int &linelength, char *&lineCStr) {
    // Fetch the next token from the input file using the existing getToken function
    Token *tokenMARIE = getToken(file, linenum, offsetnum, linelength, lineCStr);

    // Define a lambda function to check if the token's value matches any of the MARIE types
    auto isValidMARIE = [](const std::string &str) {
        const std::string validTypes = "MARIE";
        return str.length() == 1 && validTypes.find(str[0]) != std::string::npos;
    };

    // Check if the fetched token is valid and conforms to the MARIE addressing types
    if (!tokenMARIE || !isValidMARIE(tokenMARIE->value)) {
        const char* errorMessage = "MARIE_EXPECTED";  // Error message for invalid or missing MARIE token
        printf("Parse Error line %d offset %d: %s\n", linenum, offsetnum, errorMessage);
        exit(EXIT_FAILURE);  // Terminate the program with an error exit code
    }

    return tokenMARIE;  // Return the valid MARIE token
}

void PassOne(std::ifstream& file) {
    std::string line;
    int linenum = 0;
    int offsetnum = 0;
    int linelength = 0;

    //module table level variables
    int moduleAddr = 0;
    int addrMem = 0;
    int moduleNum = 0;


    char* lineCStr = nullptr; // For use with strtok
    Token* token; 

    file.clear(); // Clear any errors or EOF flags
    file.seekg(0, std::ios::beg); // Reset file pointer to the beginning of the file

    // Read each line from the file
    while (!file.eof()) {
        Module newModule;
        newModule.baseAddress = moduleAddr;
        // std::cout << "module addressessss " << moduleAddr << std::endl;
        // std::vector<Symbol> symbolsInDef; //to check later if symbols in deflist have been used

        //reading deflist
        int defCount = readInt(file, linenum, offsetnum, linelength, lineCStr);

        if (defCount >= 0 && defCount <= 16) {
            newModule.definitionCount = defCount;
            for (int idx = 0; idx < defCount; idx++) {
                Token* symbolToken = readSymbol(file, linenum, offsetnum, linelength, lineCStr);
                int relativeAddr = readInt(file, linenum, offsetnum, linelength, lineCStr);
                Symbol definedSymbol(symbolToken->value, moduleAddr, relativeAddr, moduleNum);

                // Begin by assuming we will add the symbol unless we find a duplicate
                symbolTable.push_back(definedSymbol);

                // Iterate through the symbol table to check for duplicates
                for (size_t i = 0; i < symbolTable.size() - 1; ++i) { // exclude the last added symbol
                    if (symbolTable[i].name == definedSymbol.name) {
                        // Found a duplicate: remove the last added symbol and issue a warning
                        symbolTable.pop_back();
                        symbolTable[i].symbolAlreadyDefined = true;
                        printf("Warning: Module %d: %s redefinition ignored\n", moduleNum, definedSymbol.name.c_str());
                        break;
                    }
                }

                // Always add the symbol to the current module's list of definitions
                definedSymbol.isInDefList = true;
                // symbolsInDef.push_back(definedSymbol);
            }
        } else if (defCount > 16) {
            printf("Parse Error line %d offset %d: TOO_MANY_DEF_IN_MODULE\n", linenum, offsetnum);
            exit(EXIT_FAILURE); // or handle the error as required
        } else {
            // Handle case where definitionCount is negative or other invalid conditions
            printf("Invalid defcount.\n");
            exit(EXIT_FAILURE);
        }

        //uselist
        int useCount = readInt(file, linenum, offsetnum, linelength, lineCStr);
        if (useCount >= 0 && useCount <= 16) {  // Validate the use count within allowed limits
            newModule.useCount = useCount;  // Assign the count of uses to the module's property
            for (int idx = 0; idx < useCount; idx++) {  // Iterate through each use
                Token *useToken = readSymbol(file, linenum, offsetnum, linelength, lineCStr);
                // Additional processing for each useToken could be added here if necessary
            }
        } else {  // Handle cases where the use count is outside acceptable boundaries
            printf("Parse Error line %d offset %d: TOO_MANY_USE_IN_MODULE\n", linenum, offsetnum);
            exit(EXIT_FAILURE);  // Exit the program due to the critical input error
        }

        //program text

        // Retrieve the count of instructions for the current module from the input file
        int numInstructions = readInt(file, linenum, offsetnum, linelength, lineCStr);
        addrMem += numInstructions;  // Update the total addressable memory used so far
        if (numInstructions >= 0 && numInstructions <= 512 - addrMem) {  // Ensure it doesn't exceed the machine size
            newModule.instructionCount = numInstructions;  // Store the count of instructions in the module
            for (int index = 0; index < numInstructions; index++) {  // Process each instruction
                Token *modeToken = readMARIE(file, linenum, offsetnum, linelength, lineCStr);  // Read the addressing mode
                int opcode = readInt(file, linenum, offsetnum, linelength, lineCStr);  // Read the opcode
                // The modeToken and opcode could be processed here if necessary
            }
            moduleBaseTable.push_back(newModule);  // Add the configured module to the module base table
            moduleAddr += numInstructions;  // Increment the module's base address by the number of instructions
        } 
        
        else {
            printf("Parse Error line %d offset %d: TOO_MANY_INSTR\n", linenum, offsetnum);
            exit(EXIT_FAILURE);
        }
        // Validate symbol addresses against the module's instruction count
        // for (const auto& currentSymbol : symbolsInDef) {
        // for (auto& symbolEntry : symbolTable) {
        //     if (symbolEntry.isInDefList /*&& /*symbolEntry.name == symbolEntry.name &&*/ /*symbolEntry.moduleNumber == moduleNum*/) {
        //         // int symbAddr = symbolEntry.relativeAddress;
        //         if (symbolEntry.relativeAddress >= numInstructions) {
        //             std::cout << "Warning: Module " << moduleNum << ": " << symbolEntry.name
        //                         << "=" << symbolEntry.relativeAddress << " valid=[0.." << numInstructions - 1
        //                         << "] assume zero relative" << std::endl;
        //             symbolEntry.relativeAddress = 0;  // Reset relative address if out of bounds
        //         }
        //     }
        // }
        // }
        moduleNum++;  // Increment the module number for the next iteration
        
    }

    file.close(); 
}

void PassTwo(std::ifstream& file) {

    int linenum = 0;
    int offsetnum = 0;
    int linelength = 0;
    int moduleAddr = 0;
    int moduleNum = 0;
    int printNum = 0;
    char *lineCStr = nullptr;
    Token *token;
    printf("\nMemory Map\n");

    while (file.good()) {
    // Reading definition count and processing each definition
    int defCount = readInt(file, linenum, offsetnum, linelength, lineCStr);
    for (int idx = 0; idx < defCount; ++idx) {
        Token *defSym = readSymbol(file, linenum, offsetnum, linelength, lineCStr);
        int val = readInt(file, linenum, offsetnum, linelength, lineCStr);
    }

    // Handling the use list
    int useCount = readInt(file, linenum, offsetnum, linelength, lineCStr);
    std::vector<Symbol> useListValues;

    for (int idx = 0; idx < useCount; ++idx) {
        Token *useSym = readSymbol(file, linenum, offsetnum, linelength, lineCStr);
        Symbol newSym(useSym->value, moduleAddr, -1, moduleNum);
        useListValues.push_back(newSym);
        // Checking if symbol is in the symbol table
        for (auto &it : symbolTable) {
            if (it.name == useSym->value) {
                it.isInUseList = true;
            }
        }
    }

        // std::cout << "finished uselist code, starting prog text" << std::endl;

        //program text

        int instrCount = readInt(file, linenum, offsetnum, linelength, lineCStr);
        // std::cout << "moduleaddr and instrcount before adding " << moduleAddr << instrCount << std::endl;
        moduleAddr += instrCount;
        // std::cout << "module address incrementation" << moduleAddr << std::endl;
        for(int idx=0; idx<instrCount; idx++){
            Token *addrMode = readMARIE(file, linenum, offsetnum, linelength, lineCStr);
            int instr = readInt(file, linenum, offsetnum, linelength, lineCStr);
            int opcode = instr / 1000;
            int operand = instr % 1000;

            //handle MARIE cases
            if(opcode <= 9){
                switch (addrMode->value[0]) {
                case 'A':{
                    if (operand > 512) {
                        printf("%0.3d: %0.4d Error: Absolute address exceeds machine size; zero used\n", printNum, opcode * 1000);
                    } else {
                        printf("%0.3d: %0.4d\n", printNum, instr);
                    }
                    break;}
                case 'I':{
                    if (operand >= 900) {
                        printf("%0.3d: %0.4d Error: Illegal immediate operand; treated as 999\n", printNum, (opcode * 1000) + 999);
                    } else {
                        printf("%0.3d: %0.4d\n", printNum, instr);
                    }
                    break;}
                case 'M':{
                    Module currModuleAddr;  
                    int updatedInstr;        // Declare the instruction variable outside for use in both scenarios

                    // Determine the current module and calculate the instruction
                    if (operand < moduleBaseTable.size() - 1) {
                        currModuleAddr = moduleBaseTable[operand];
                        updatedInstr = (opcode * 1000) + currModuleAddr.baseAddress;
                        printf("%0.3d: %0.4d\n", printNum, updatedInstr);
                    } else {
                        // Handle cases where operand is out of the module table bounds
                        currModuleAddr = moduleBaseTable[0];  // Use the first module as the default
                        updatedInstr = (opcode * 1000) + currModuleAddr.baseAddress;
                        printf("%0.3d: %0.4d Error: Illegal module operand ; treated as module=0\n", printNum, updatedInstr);
                    }
                    break;}
                case 'R':{
                    // Handling relative addressing within the module
                    Module currModuleAddr = moduleBaseTable[moduleNum];  // Current module address
                    if (operand < currModuleAddr.instructionCount) {
                        int updatedInstr = (opcode * 1000) + (operand + currModuleAddr.baseAddress);
                        printf("%0.3d: %0.4d\n", printNum, updatedInstr);
                    } else {
                        operand = 0;  // Reset operand if it exceeds the module size
                        int updatedInstr = (opcode * 1000) + (operand + currModuleAddr.baseAddress);
                        printf("%0.3d: %0.4d Error: Relative address exceeds module size; relative zero used\n", printNum, updatedInstr);
                    }
                    break;}
                case 'E':{
                    int updatedInstr = (opcode * 1000);  // Prepare the base for the updated instruction

                    // Check if the operand is within the bounds of the use list
                    if (operand < useListValues.size()) {
                        Symbol& tokenSymbol = useListValues[operand];
                        tokenSymbol.definedAndUsed = true;  // Mark the symbol as used

                        bool symbolFound = false;  // Flag to check if the symbol was found and updated
                        // Iterate through the symbol table to find and update the instruction
                        for (auto &it : symbolTable) {
                            if (it.name == tokenSymbol.name) { // Ensure correct symbol and not multiply defined
                                updatedInstr += (it.relativeAddress + it.moduleAddress);
                                printf("%0.3d: %0.4d\n", printNum, updatedInstr);
                                symbolFound = true;
                                break;  // Exit loop after updating
                            }
                        }

                        // Handle case where symbol is not found or multiply defined without resolution
                        if (!symbolFound) {
                            printf("%0.3d: %0.4d Error: %s is not defined; zero used\n", printNum, updatedInstr, tokenSymbol.name.c_str());
                        }
                    } else {
                        // Handle out of bounds operand
                        printf("%0.3d: %0.4d Error: External operand exceeds length of uselist; treated as relative=0\n", printNum, updatedInstr);
                    }
                    break;}
                // default:
                //     printf("%0.3d: %0.4d Error: Invalid addressing mode\n", printCounter, 9999);
                //     break;
                }
            }

            else {
                int updatedInstr = 9999;
                printf("%0.3d: %0.4d Error: Illegal opcode; treated as 9999\n", printNum, updatedInstr); 
            }
            printNum++;
        }
        moduleAddr += instrCount;
        // std::cout << "module address incrementation" << moduleAddr << std::endl;

        for (size_t idx = 0; idx < useListValues.size(); ++idx) {
            const auto &symbol = useListValues[idx];
            if (!symbol.definedAndUsed) {
                printf("Warning: Module %d: uselist[%zu]=%s was not used\n", symbol.moduleNumber, idx, symbol.name.c_str());
            }
        }
        moduleNum++;

    }

    // Checking all symbols in the symbol table to see if any were defined but never used
    for ( auto &symbol : symbolTable) {
        if (!symbol.isInUseList) {
            printf("\nWarning: Module %d: %s was defined but never used\n", symbol.moduleNumber, symbol.name.c_str());
        }
    }
    file.close();
}

void printSymbolTable() {
    std::cout << "Symbol Table\n";
    for (const auto& symbol : symbolTable) {
        // std::cout << "module address" << symbol.moduleAddress<< std::endl;
        // std::cout << "module nunmber" << symbol.moduleNumber <<std::endl;
        // std::cout << "relative address" << symbol.relativeAddress << std::endl;
        std::cout << symbol.name << "=" << (symbol.moduleAddress + symbol.relativeAddress);
        if (symbol.symbolAlreadyDefined) {
            std::cout << " Error: This variable is multiple times defined; first value used";
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    std::string filename = argv[1];

    std::ifstream file1(filename);
    if (!file1.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }

    PassOne(file1);  // Ensure no read operations happen before this call
    file1.close();

    std::ifstream file2(filename);
    if (!file2.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }
    printSymbolTable();
    
    PassTwo(file2);

    

    return 0;
}
