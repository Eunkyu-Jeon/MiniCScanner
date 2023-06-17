/***************************************************************
*      scanner routine for Mini C language                    *
***************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "Scanner.h"

extern FILE *sourceFile;                       // miniC source program


int superLetter(char ch);
int superLetterOrDigit(char ch);
int getNumber(char firstCharacter);
int hexValue(char ch);
void lexicalError(int n);

char temp[ID_LENGTH];

bool isDoubleNum = false;
int lineNum = 1;
int colNum = 1;



char *tokenName[] = {
	"!",        "!=",      "%",       "%=",     "%ident",   "%number",
	/* 0          1           2         3          4          5        */
	"&&",       "(",       ")",       "*",      "*=",       "+",
	/* 6          7           8         9         10         11        */
	"++",       "+=",      ",",       "-",      "--",	    "-=",
	/* 12         13         14        15         16         17        */
	"/",        "/=",      ";",       "<",      "<=",       "=",
	/* 18         19         20        21         22         23        */
	"==",       ">",       ">=",      "[",      "]",        "eof",
	/* 24         25         26        27         28         29        */
	//   ...........    word symbols ................................. //
	/* 30         31         32        33         34         35        */
	"const",    "else",     "if",      "int",     "return",  "void",
	/* 36         37         38        39       40       41            */
	"while",    "{",        "||",       "}",	"char",	"double",	
	/*42     43      44      45        46        47          48         */
	"for",	"do",	"goto",	"switch",	"case",	"break",	"default", "."
	/*49     50      51      52           53     54            55       56  */
	
};

char *keyword[NO_KEYWORD] = {
	"const",  "else",    "if",    "int",    "return",  "void",	"while",  "char",	"double",	"for",	"do",	"goto",
	"switch",	"case",	"break",	"default"
};

enum tsymbol tnum[NO_KEYWORD] = {
	tconst,    telse,     tif,     tint,     treturn,   tvoid,     twhile, tchar, tdouble, tfor, tdo, tgoto,
	tswitch, 	tcase,	tbreak, 	tdefault
};

struct tokenType scanner()
{
	struct tokenType token;
	int i, index;
	char ch, id[ID_LENGTH];

	token.number = tnull;

	do {
		while (isspace(ch = fgetc(sourceFile))) {
			
			if (ch=='\n') {
				lineNum++;
				colNum = 1;
			} else {
				colNum++;
			}
		};	// state 1: skip blanks

		if (superLetter(ch)) { // identifier or keyword
			
			token.lineNum = lineNum;
			token.colNum = colNum;
			
			i = 0; //index 조정
			
			do {
				if (i < ID_LENGTH) { //식별자의 길이보다 작은 경우
					id[i++] = ch;
				}
				ch = fgetc(sourceFile);
			} while (superLetterOrDigit(ch));

			if (i >= ID_LENGTH) lexicalError(1); //식별자의 길이보다 크거나 같은경우
			id[i] = '\0';

			ungetc(ch, sourceFile);  //  retract
									 // find the identifier in the keyword table
			for (index = 0; index < NO_KEYWORD; index++)
				if (!strcmp(id, keyword[index])) break;
			if (index < NO_KEYWORD)    // found, keyword exit
				token.number = tnum[index];
			else {                     // not found, identifier exit
				token.number = tident;
				strcpy(token.id, id);
			}
		}  // end of identifier or keyword
		else if (isdigit(ch)) {  // number
			token.lineNum = lineNum;
			token.colNum = colNum;
			token.number = tnumber;
			//token.value.num = getNumber(ch);

			char temp[ID_LENGTH];
			double tmp = getNumber(ch);

			// strcpy(token.id, temp);
			if (tmp >= 99999999999)
				strcpy(token.id, temp);
			else if (isDoubleNum)
			{
				sprintf(temp, "%lf", tmp);
				strcpy(token.id, temp);
			}
			else
			{
				sprintf(temp, "%d", (int)tmp);
				strcpy(token.id, temp);
			}
			memset(temp, '\0', sizeof(temp));
		}
		else switch (ch) {  // special character
		case '\\' :
			char tmp[ID_LENGTH];
			int idx = 0;
			ch = fgetc(sourceFile);
			colNum++;
			tmp[idx] = ch;
			idx++;
			tmp[idx] = '\0';
			strcpy(token.id,tmp);
			token.lineNum = lineNum;
			token.colNum = colNum;
			break;

		case '"' :
			char tmp[ID_LENGTH];
			int idx=0;
			ch = fgetc(sourceFile);
			colNum++;
			while (ch!='\'') {
				if (ch=='\n') {
					ch=' ';
				}
				tmp[idx]=ch;
				idx++;
				ch=fgetc(sourceFile);
				colNum++;
			}
			tmp[idx]='\0';
			strcpy(token.id,tmp);
			token.lineNum = lineNum;
			token.colNum = colNum;
			break;
		case '/':
			ch = fgetc(sourceFile);
			
			colNum++;
			
			if (ch == '*')	{ // /* text comment
				char tmp[COMMENT_LENGTH];
				int idx=0;
				ch = fgetc(sourceFile);
				colNum++;

				if(ch =='*') {
					token.isDocument=2 // /** documented comment

					do {
					while (ch != '*') {
						
						if (ch=='\n') {
							ch=' ';
						}
						tmp[idx]=ch;
						idx++;
						ch = fgetc(sourceFile);
						colNum++;
					}


					ch = fgetc(sourceFile);
					colNum++;
					} while (ch != '/');

					tmp[idx] = '\0';
					strcpy(token.comment,tmp);
					ungetc(ch,sourceFile);
				}
				else { // /* comment
					do {
						while (ch!='*') {
							if (ch=='\n') ch=' ';
							tmp[idx]=ch;
							idx++;
							ch=fgetc(sourceFile);
							colNum++;
						}
						ch=fgetc(sourceFile);
						colNum++;
					}while (ch!='/');
					tmp[idx] = '\0';
					strcpy(token.comment,tmp);
					ungetc(ch,sourceFile);
					token.isDocument=1;
				}
			}		// text comment
				
			else if (ch == '/')	{  // //singgle line comment
				if (ch=='/') { // ///documented comment
					token.isDocument =2;
				} 
				else {
					token.isDocument =1;
				}				
				char tmp[COMMENT_LENGTH];
				int idx=0;

				do {
					ch=fgetc(sourceFile);
					colNum++;
					if (ch!='\n') {
						tmp[idx]=ch;
						idx++;
					}
				} while (ch!='\n');
				tmp[idx] = '\0';
				strcpy(token.comment,tmp);
				ungetc(ch,sourceFile);
			}	
				
			else if (ch == '=')  token.number = tdivAssign;
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"/");
				token.number = tdiv;
				ungetc(ch, sourceFile); // retract
			}
			break;
			
		case '!':
			ch = fgetc(sourceFile);
			if (ch == '=')  {
				token.lineNum = lineNum;
				token.colNum = colNum;
				strcpy(token.id,"!=");				
				token.number = tnotequ;
				colNum++;
			}
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"!");
				token.number = tnot;
				ungetc(ch, sourceFile); // retract
			}
			break;
		case '%':
			ch = fgetc(sourceFile);
			if (ch == '=') {
				token.lineNum = lineNum;
				token.colNum = colNum;
				strcpy(token.id,"%=");
				colNum++;
				token.number = tremAssign;
			}
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"%");
				token.number = tremainder;
				ungetc(ch, sourceFile);
			}
			break;
		case '&':
			ch = fgetc(sourceFile);
			if (ch == '&')  {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				token.number = tand;
				strcpy(token.id,"&&");
			}
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"&");
				lexicalError(2);
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '*':
			ch = fgetc(sourceFile);
			if (ch == '=') {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				token.number = tmulAssign;
				strcpy(token.id,"*=");
			}
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"*");
				token.number = tmul;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '+':
			ch = fgetc(sourceFile);
			if (ch == '+')  {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"++");
				token.number = tinc;
			}
			else if (ch == '=') {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"+=");
				token.number = taddAssign;
			}
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"+");
				token.number = tplus;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '-':
			ch = fgetc(sourceFile);
			if (ch == '-')  {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"--");
				token.number = tdec;
			}
			else if (ch == '=') {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"-=");
				token.number = tsubAssign;
			}
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"&");
				token.number = tminus;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '<':
			ch = fgetc(sourceFile);
			if (ch == '=') {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"<=");
				token.number = tlesse;
			}
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"<");
				token.number = tless;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '=':
			ch = fgetc(sourceFile);
			if (ch == '=')  {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"==");
				token.number = tequal;
			}
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"=");
				token.number = tassign;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '>':
			ch = fgetc(sourceFile);
			if (ch == '=') {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,">=");
				token.number = tgreate;
			}
			else {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,">");
				token.number = tgreat;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '|':
			ch = fgetc(sourceFile);
			if (ch == '|')  {
				token.lineNum = lineNum;
				token.colNum = colNum;
				colNum++;
				strcpy(token.id,"||");
				token.number = tor;
			}
			else {
				lexicalError(3);
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '(': 
			token.number = tlparen;         
			token.lineNum = lineNum;
			token.colNum = colNum;
			colNum++;
			strcpy(token.id,"&");
			break;
		case ')': token.number = trparen;         
			token.lineNum = lineNum;
			token.colNum = colNum;
			colNum++;
			strcpy(token.id,"&");
			break;
		case ',': token.number = tcomma;          
			token.lineNum = lineNum;
			token.colNum = colNum;
			colNum++;
			strcpy(token.id,"&");
			break;
		case ';': token.number = tsemicolon;      
			token.lineNum = lineNum;
			token.colNum = colNum;
			colNum++;
			strcpy(token.id,"&");
			break;
		case '[': token.number = tlbracket;       
			token.lineNum = lineNum;
			token.colNum = colNum;
			colNum++;
			strcpy(token.id,"&");
			break;
		case ']': token.number = trbracket;       
			token.lineNum = lineNum;
			token.colNum = colNum;
			colNum++;
			strcpy(token.id,"&");
			break;
		case '{': token.number = tlbrace;         
			token.lineNum = lineNum;
			token.colNum = colNum;
			colNum++;
			strcpy(token.id,"&");
			break;
		case '}': token.number = trbrace;         
			token.lineNum = lineNum;
			token.colNum = colNum;
			colNum++;
			strcpy(token.id,"&");
			break;
		case EOF: token.number = teof;            break;
		default: {
			printf("Current character : %c", ch);
			lexicalError(4);
			break;
		}

		} // switch end
	} while (token.number == tnull);
	return token;
} // end of scanner

void lexicalError(int n)
{
	printf(" *** Lexical Error : ");
	switch (n) {
	case 1: printf("an identifier length must be less than 12.\n");
		break;
	case 2: printf("next character must be &\n");
		break;
	case 3: printf("next character must be |\n");
		break;
	case 4: printf("invalid character\n");
		break;
	}
}

int superLetter(char ch)
{
	if (isalpha(ch) || ch == '_') return 1;
	else return 0;
}

int superLetterOrDigit(char ch)
{
	if (isalnum(ch) || ch == '_') return 1;
	else return 0;
}

int getNumber(char firstCharacter)
{
	double num = 0;
	int value;
	char ch;
	int idx=0;
	int point=1;
	char temp[COMMENT_LENGTH];

	temp[idx] = firstCharacter;
	idx++;

	if (firstCharacter == '0') {
		ch = fgetc(sourceFile);
		colNum++;

		if ((ch == 'X') || (ch == 'x')) {		// hexa decimal
			while ((value = hexValue(ch = fgetc(sourceFile))) != -1)
				num = 16 * num + value;
				colNum++;
				temp[idx]=ch;
				idx++;
		}
		else if ((ch >= '0') && (ch <= '7'))	// octal
			do {
				num = 8 * num + (int)(ch - '0');
				ch = fgetc(sourceFile);
				colNum++;
				temp[idx]=ch;
				idx++;
			} while ((ch >= '0') && (ch <= '7'));
		else if (ch=='.') {
			isDoubleNum=true;
			do {
				if (ch>=0 && ch<='9') {
					num += (double)(ch - '0') *(1.0 / (double)pow(10, point));
					point++;
				}
				ch=fgetc(sourceFile);
				colNum++;
			if (ch!='\n' && isdigit(ch)) {
				temp[idx]=ch;
				idx++;
			}
			colNum++;


			}while (isdigit(ch) || ch=='.');
			
			
		}
		else num = 0;						// zero
	}
	else {									// decimal
		ch = firstCharacter;
		do {
			if (ch=='.') {
				isDoubleNum=true;
			do {
				if (ch>=0 && ch<='9') {
				num += (double)(ch - '0') *(1.0 / (double)pow(10, point));
				point++;
			}
			ch=fgetc(sourceFile);
			colNum++;
			if (ch!='\n' && isdigit(ch)) {
				temp[idx]=ch;
				idx++;
			}
			colNum++;


			}while (isdigit(ch) || ch=='.');
			}
			else {
				num = 10 * num + (int)(ch - '0');
			ch = fgetc(sourceFile);
			}
			
		} while (isdigit(ch)||ch=='.');
	}
	ungetc(ch, sourceFile);  /*  retract  */
	return num;
}

int hexValue(char ch)
{
	switch (ch) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return (ch - '0');
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		return (ch - 'A' + 10);
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		return (ch - 'a' + 10);
	default: return -1;
	}
}

// void printToken(struct tokenType token)
// {
// 	if (token.number == tident)
// 		printf("number: %d, value: %s\n", token.number, token.id);
// 	else if (token.number == tnumber)
// 		printf("number: %d, value: %d\n", token.number, token.value.num);
// 	else
// 		printf("number: %d(%s)\n", token.number, tokenName[token.number]);

// }

void printToken(struct tokenType token) {
	if (token.isDocument==0) {
		printf("Token -> %s (%d %s %s %s %d)\n", token.id, token.number, token.id, token.fileName, token.lineNum, token.colNum);
	}
	else if (isDocument==1) {
		printf("comment : %s", token.comment);
	}
	else {
		printf("Documented comment : %s", token.comment);
	}
}