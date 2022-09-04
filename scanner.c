/*
* Abi Risher
* CS421
* 4/6/2020
* Scanner
*/
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
* Input strings can be at max 80 characters with 1 character reserved for a new
* line character 
*/
#define LINESIZE 81
/* Tokens can be at max 12 characters long */
#define TOKENSIZE 12
/* There are 23 reserved keywords in this particular pascal variant */
#define KEYWORDNUM 23
/* There is a maximum of 32 identifiers for any given program */
#define MAXSYMBOLS 32
/* Input source files can be at max 128 characters long */
#define SOURCELENGTH 128
/* Location containing reserved keywords followed by their ids */
#define TOKENTABLE "TokenTable.txt"

struct token
{
	int code;
	char name[TOKENSIZE];
};
struct identifier
{
	int uses;
	char name[TOKENSIZE];
};

bool contextSensitivity(int, int, char *, char *);
bool isStopper(char);
bool retrieveFiles(int, char **, FILE **, FILE **);
bool validIdentifier(int, char *);
bool validInt(int, char *);

int commentMode(int, int, int *, char *, FILE *);
int getTokenNum(int, int, char *, struct token *);
int getatoken(int, int, int, int, char *, char *);

void fillTokenTable(int, FILE *, struct token *);
void preface(void);
void tokenOuputsVariants(int, char *, struct token *);

int hashCode(struct identifier, int);
bool hashSearch(struct identifier *, struct identifier, int);
void hashInsert(struct identifier *, struct identifier, int, int);
void printHashTable(struct identifier *);

int main(int argc, char *argv[])
{
	preface();
	FILE *sourceFile_p;
	FILE *tokenFile_p;

	if (retrieveFiles(argc, argv, &sourceFile_p, &tokenFile_p))
	{
		bool foundBegin = false;
		int strIndex;
		int lineCount = 1;
		int tokenNum;
		char strIn[LINESIZE];
		char tokIn[TOKENSIZE];
		struct token table[KEYWORDNUM];
		struct identifier idTable[MAXSYMBOLS] = {0, ""};

		fillTokenTable((sizeof(table) / sizeof(table[0])),
					   tokenFile_p, table);
		memset(strIn, '\0', sizeof(strIn));

		while (fgets(strIn, sizeof(strIn), sourceFile_p))
		{
			if ((!(strIn[LINESIZE - 2] == '\0' || strIn[LINESIZE - 2] == '\n'))
				 && strIn[LINESIZE - 1] != '\n')
			{
				printf("Error - Line longer than 80 characters.\n");
				strIn[LINESIZE - 1] = '\n';
			}
			strIndex = 0;

			/* 
			* Reads a line while a counter is not at the end of a string of
			* max size 81 characters
			*/
			while (!(strIn[strIndex] == '\n' || strIn[strIndex] == '\0') &&
				   strIndex < sizeof(strIn))
			{
				memset(tokIn, '\0', sizeof(tokIn));
				strIndex = getatoken(sizeof(strIn), sizeof(tokIn),
									 strIndex, lineCount, strIn, tokIn);
				tokenNum = getTokenNum((sizeof(table) / sizeof(table[0])),
									   sizeof(tokIn), tokIn, table);

				/* 
				* If the recieved token number is 0, signals to go into 
				*   comment mode 
				*/
				if (tokenNum == 0)
				{
					strIndex = commentMode(strIndex, sizeof(strIn),
										   &lineCount, strIn,
										   sourceFile_p);
					if (strIndex != 0)
					{
						strIndex = getatoken(sizeof(strIn), sizeof(tokIn),
											 strIndex, lineCount, strIn, 
											 tokIn);
						tokenNum = getTokenNum((sizeof(table) /
												sizeof(table[0])),
											   sizeof(tokIn), tokIn,
											   table);
					}
				}
				else
				{
					if (tokenNum == -1)
					{
						printf("\tImproperly formatted identifier ^%s\n", 
								tokIn);
					}
					else
					{
						tokenOuputsVariants(tokenNum, tokIn, table);
						if (strcmp(tokIn, "") != 0)
						{
							printf("%s", tokIn);
							if (tokIn[TOKENSIZE] != '\0')
							{
								printf("\0");
							}
							printf("\n", tokIn);
						}
						if (strcmp(tokIn, "BEGIN") == 0)
						{
							foundBegin = true;
						}
						if (tokenNum == idTokenNum(table) && !foundBegin)
						{
							struct identifier id = {0, ""};
							strncpy(id.name, tokIn, sizeof(id.name));
							hashInsert(idTable, id, 
									   hashCode(id, sizeof(id.name)), 
									   sizeof(id.name));
						}
						else if (tokenNum == idTokenNum(table) && foundBegin)
						{
							bool idFound = false;
							struct identifier id = {0, ""};
							strncpy(id.name, tokIn, sizeof(id.name));
							idFound = hashSearch(idTable, id, 
												 hashCode(id, 
												 		  sizeof(id.name)));
						}
					} //end of else statement
				} //end of else statement
			} //End of while loop
			memset(strIn, '\0', sizeof(strIn));
			lineCount++;
		} //End of while loop
		fclose(sourceFile_p);
		fclose(tokenFile_p);
		printHashTable(idTable);
	} //End of if statement
} //End of main

//-----------------------------------------------------------------------------
/* 
* Checks characters for context-sensitivity in these instances:
* ( & (*, : & :=, * & *)
*/
bool contextSensitivity(int strIndex, int tokIndex,
						char *tokIn_p, char *strIn_p)
{
	bool contextFound = false;
	switch (tokIn_p[tokIndex])
	{
	case ('('):
		if (strIn_p[strIndex + 1] == '*')
		{
			contextFound = true;
		}
		break;
	case (':'):
		if (strIn_p[strIndex + 1] == '=')
		{
			contextFound = true;
		}
		break;
	case ('*'):
		if (strIn_p[strIndex + 1] == ')')
		{
			contextFound = true;
		}
		break;
	default:
		contextFound = false;
		break;
	} /* End of switch statement*/
	return contextFound;
} //End of function

//-----------------------------------------------------------------------------
/* 
* Returns TRUE if the character is a stopper
*/
bool isStopper(char in)
{
	bool stopperFound;

	/* 
	* Checks input character for if it is considered a stopper
	* Stoppers include the characters ' ;:,+-*()\n\0'
	*/
	if (in == ' ' ||
		in == ';' ||
		in == ':' ||
		in == ',' ||
		in == '+' ||
		in == '-' ||
		in == '*' ||
		in == '(' ||
		in == ')' ||
		in == '\n' ||
		in == '\0')
	{
		stopperFound = true;
	}
	else
	{
		stopperFound = false;
	}

	return stopperFound;
} /* End of function */

//-----------------------------------------------------------------------------
/* 
* Function used to open the 'TokenTable.txt' file and source file as input by
* the user. Will return true if both files are openned succesfully.
*/
bool retrieveFiles(int argc, char **argv_p, FILE **sourceFile_p, 
				   FILE **tokenFile_p)
{
	bool filesOpenned;
	if (*tokenFile_p = fopen(TOKENTABLE, "r"))
	{
		char sourceLocation[SOURCELENGTH];
		if (argc != 2)
		{
			printf("Enter program to compile: ");
			gets(sourceLocation);
		}
		else
		{
			strncpy(sourceLocation, argv_p[1], sizeof(sourceLocation));
		}
		if (*sourceFile_p = fopen(sourceLocation, "r"))
		{
			filesOpenned = true;
		}
		else
		{
			printf("Error opening file \"%s\", exiting.\n", sourceLocation);
			filesOpenned = false;
			fclose(*tokenFile_p);
		}
	} //End of if statement
	else
	{
		printf("Token table not found, exiting.\n");
		filesOpenned = false;
	}

	return filesOpenned;
} //End of function

//-----------------------------------------------------------------------------
/* 
* Returns TRUE if the string is a valid identifier
* Valid identifiers begin with an alphabetic, and can contain up to 11
*	more alphanumeric
*/
bool validIdentifier(int strSize, char *strIn_p)
{
	bool validId = true;

	if (isalpha(strIn_p[0]) == 0)
	{
		validId = false;
	}
	else
	{
		int count;
		for (count = 1; count < strSize; count++)
		{
			if (!(isalnum(strIn_p[count]) ||
				  strIn_p[count] == '\0'))
			{
				validId = false;
			}
		}
	}

	return validId;
} /* End of function */

//-----------------------------------------------------------------------------
/*
* Returns TRUE if the string is a valid integer
*/
bool validInt(int strSize, char *strIn_p)
{
	bool validInt = true;
	int count;

	for (count = 0; count < strSize; count++)
	{
		if (!(isdigit(strIn_p[count]) ||
			  strIn_p[count] == '\0'))
		{
			validInt = false;
		}
	}

	return validInt;
}

//-----------------------------------------------------------------------------
/*
* Returns the index of the string read from the source file. Will progress
*   through the file until the character * followed by the character ) are 
*   found.
*/

int commentMode(int strIndex,
				int strSize,
				int *lineCount_p,
				char *strIn_p,
				FILE *sourceFile_p)
{
	bool endLoop = false;
	while (!endLoop)
	{
		if (strIn_p[strIndex] == '*' &&
			strIndex < strSize - 1)
		{
			if (strIn_p[strIndex + 1] == ')')
			{
				endLoop = true;
			}
			else
			{
				strIndex++;
			}
		}
		else if (strIn_p[strIndex] == '\0' ||
				 strIn_p[strIndex] == '\n' ||
				 strIndex == strSize)
		{
			memset(strIn_p, '\0', sizeof(strIn_p));
			if (fgets(strIn_p, strSize, sourceFile_p) != NULL)
			{
				*lineCount_p = *lineCount_p + 1;
				printf("%d\t%s", *lineCount_p, strIn_p);
				strIndex = 0;
			}
			else
			{
				endLoop = true;
			}
		}
		else
		{
			strIndex++;
		}
	} // End of while loop

	return strIndex;
} //End of function

//-----------------------------------------------------------------------------
/*
* Returns the number reserved by each keyword for an input token. Numbers are 
*   stored in an array of token structs.
*/
int getTokenNum(int tableSize,
				int tokSize,
				char *tokIn_p,
				struct token *table_p)
{
	bool tokenFound = false;
	int code = 1;
	int count = 0;

	while (!tokenFound)
	{
		// Checking to see if the token is a reserved keyword
		if (strcmp(tokIn_p, table_p[count].name) == 0)
		{
			tokenFound = true;
		}
		// Checking to see if the token is a valid identifier
		else if (strcmp("id", table_p[count].name) == 0 &&
				 validIdentifier(tokSize, tokIn_p))
		{
			tokenFound = true;
		}
		// Checking to see if the token is a valid integer
		else if (strcmp("int", table_p[count].name) == 0 &&
				 validInt(tokSize, tokIn_p))
		{
			tokenFound = true;
		}
		// Signalling to enter comment mode
		else if (strcmp("(*", tokIn_p) == 0)
		{
			tokenFound = true;
			code = 0;
		}
		else
		{
			count++;
			if (count >= tableSize)
			{
				code = -1;
				tokenFound = true;
			}
		}
	} /* End of while loop */

	if (!(code == 0 ||
		  code == -1))
	{
		code = table_p[count].code;
	}

	return code;
} /* End of function */

//-----------------------------------------------------------------------------
/*
* Returns the index of the string after reading a token.
*/
int getatoken(int strSize, int tokSize, int strIndex, int lineCount,
			  char *strIn_p, char *tokIn_p)
{

	bool stopperFound = false;
	int tokIndex = 0;
	if (strIndex == 0)
	{
		printf("%d\t%s", lineCount, strIn_p);
	}

	while (strIndex < strSize &&
		   tokIndex < tokSize &&
		   !stopperFound)
	{
		if (strIn_p[strIndex] == ' ' ||
			strIn_p[strIndex] == '\t')
		{
			strIndex++;
			stopperFound = true;
		}
		else if (tokIndex == 0 && isStopper(strIn_p[strIndex]))
		{
			tokIn_p[tokIndex] = strIn_p[strIndex];

			if ((strIndex < strSize - 1) && 
				(contextSensitivity(strIndex, tokIndex, tokIn_p, strIn_p)))
			{
				strIndex++;
				tokIndex++;
				tokIn_p[tokIndex] = strIn_p[strIndex];
			}
			stopperFound = true;
			strIndex++;
		} /* End of else if clause */
		else
		{
			if (isStopper(strIn_p[strIndex]))
			{
				stopperFound = true;
			}
			else
			{
				tokIn_p[tokIndex] = toupper(strIn_p[strIndex]);
				if (strIndex < strSize - 1 &&
					strcmp(tokIn_p, "END") == 0)
				{
					if (strIn_p[strIndex + 1] == '.')
					{
						strIndex++;
						tokIndex++;
						tokIn_p[tokIndex] = strIn_p[strIndex];
						stopperFound = true;
						;
					}
				}
				strIndex++;
				tokIndex++;
			}
		} /* End of else clause */
	} /* End of while loop */

	return strIndex;
} /* End of function */

//-----------------------------------------------------------------------------
/*
* Converts a string into an integer total, and then returns a integer for use
*   in a hash table.
*/
int hashCode(struct identifier item, int idNameSize)
{
	int counter;
	int convertedString = 0;
	int hashCode;
	for (counter = 0; counter < idNameSize; counter++)
	{
		convertedString += item.name[counter];
	}
	hashCode = convertedString % MAXSYMBOLS;
	return hashCode;
}

//-----------------------------------------------------------------------------
/*
* Returns the token code for ids
*/
int idTokenNum(struct token *table)
{
	bool idFound = false;
	int counter = 0;
	while (counter < KEYWORDNUM && !idFound)
	{
		if (strcmp(table[counter].name, "id") == 0)
		{
			idFound = true;
			counter++;
		}
		else
		{
			idFound = false;
			counter++;
		}
	}
	return counter;
}

//-----------------------------------------------------------------------------
/*
* Populates an array of token structs through use of the file tokenFile_p.
*   tokenFile_p is currentyl hardcoded as "TokenTable.txt"
*/
void fillTokenTable(int tableSize, FILE *tokenFile_p, struct token *table_p)
{
	int count = 0;
	char buf[16];
	char *splitBuf_p;

	while (fgets(buf, sizeof(buf), tokenFile_p) && count < tableSize)
	{
		splitBuf_p = strtok(buf, " \n\t");
		strncpy(table_p[count].name, splitBuf_p, sizeof(table_p[count].name));
		if (splitBuf_p = strtok(NULL, " \n\t"))
		{
			table_p[count].code = atoi(splitBuf_p);
		}
		else
		{
			table_p[count].code = count + 1;
		}
		count++;
	}
} //End of function

//-----------------------------------------------------------------------------
/* 
* Used to output starting information to the user.
*/
void preface(void)
{
	printf("This program will take text files, and search them for tokens\n");
	printf("that are reserved within a 'TokenTable.txt' file.\n");
	printf("Lines should be at most 80 characters long.\n");
	printf("Identifiers can be at most 12 characters long, starting with\n");
	printf("an alphabetic letter, followed by at most 11 more alphanumeric\n");
	printf("characters.\n");
	printf("There is a maximum number of 32 identifiers for any given ");
	printf("program.\n");
};

//-----------------------------------------------------------------------------
/* 
* Used to output in instances where a id or number token is found, meaning that
*   a ^ character for ids and a # character for number tokens. Will also output
*   a message for if a found token is an improperly formatted id.
*/
void tokenOuputsVariants(int tokenNum, char *tokIn_p, struct token *table_p)
{
	if (!((strcmp(tokIn_p, "") == 0)))
	{
		printf("\tToken #%d\t", tokenNum);
		if (strcmp("id", table_p[tokenNum - 1].name) == 0)
		{
			printf("^");
		}
		else if (strcmp("int", table_p[tokenNum - 1].name) == 0)
		{
			printf("#");
		}
	}
}

//-----------------------------------------------------------------------------
/*
* Searches a hash table specified by 'idList' for an identifier specified by an 
*	input 'id', starting at the 'hashCode' index. Returns TRUE if that id is 
*	found in the list.
*/
bool hashSearch(struct identifier *idList_p, struct identifier id, 
				int hashCode)
{
	bool foundString = false;
	bool loopedThrough = false;

	int stopCounter = hashCode - 1;
	if (stopCounter < 0)
	{
		stopCounter = MAXSYMBOLS;
	}
	int counter = hashCode;
	while (!loopedThrough)
	{
		if (strcmp(idList_p[counter].name, id.name) == 0)
		{
			foundString = true;
			loopedThrough = true;
			idList_p[counter].uses++;
		}
		else if (counter == stopCounter)
		{
			printf("Error - Undeclared identifier '%s'.\n", id.name);
			loopedThrough = true;
			foundString = false;
		}
		else if (counter == MAXSYMBOLS - 1)
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
	} //End of while loop
	return foundString;
} //End of function

//-----------------------------------------------------------------------------
/*
* Inserts an input 'id' into an input 'idTable' using that 
*/
void hashInsert(struct identifier *idTable_p, struct identifier id, 
				int hashCode, int idNameSize)
{
	bool endLoop = false;
	int stopCounter = hashCode - 1;
	if (stopCounter < 0)
	{
		stopCounter = MAXSYMBOLS;
	}
	int counter = hashCode;
	while (!endLoop)
	{
		if (strcmp(idTable_p[counter].name, "") == 0)
		{
			strncpy(idTable_p[counter].name, id.name, idNameSize);
			endLoop = true;
		}
		else if (strcmp(idTable_p[counter].name, id.name) == 0)
		{
			printf("Error - Duplicate identifier definition.\n");
			endLoop = true;
		}
		else if (counter == stopCounter)
		{
			printf("Error - Too many identifiers declared.\n");
			endLoop = true;
		}
		else if (counter == MAXSYMBOLS)
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
	} //End of while loop
} //End of function

//-----------------------------------------------------------------------------
/*
* Returns the index of the string read from the source file. Will progress
*   through the file until the character * followed by the character ) are 
*   found.
*/
void printHashTable(struct identifier *idTable_p)
{
	int counter;
	for (counter = 0; counter < MAXSYMBOLS; counter++)
	{
		if (strcmp(idTable_p[counter].name, "") != 0 && 
			idTable_p[counter].uses == 0)
		{
			printf("\nError - Identifier '%s' unused.", 
					idTable_p[counter].name);
		}
	}
	printf("\n-----------Hash Table-----------");
	printf("\n         Index   Identifier\n");
	for (counter = 0; counter < MAXSYMBOLS; counter++)
	{
		printf("%14d   %s\n", counter, idTable_p[counter].name);
	}
}
