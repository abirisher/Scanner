# Scanner
* This program intends to perform a lexical analysis of a source file as input
*   by the user. This input source file can be input as a command line argument,
*   or as input during the lifetime of the program.
* This program will attempt to open a 'TokenTable.txt' and that input source
*   file. Should either file be missing, an appropriate output message will be
*   displayed, and the program will close.
* This program will then construct a token table using the 'TokenTable.txt'
*   file. Lines will be read from the source file into a string of max size 81
*   characters with one reserved character for the new line character '\n'. The
*	program will then progress through that string stripping tokens into a
*	seperate string of size 12 for tokens which are allowed to start with an
*	alphabetic character, and may contain up to 11 more alphanumeric
*	characters. That token will then be compared against the token table, and
*	the proper id will be found for that specific token. The program will ouput
* 	information in the form:
*	  (LINE NUMBER)	(READ LINE)
*					TOKEN #(TOKEN ID)
