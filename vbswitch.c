/* 
 * File:    vbswitch.c
 * 
 * Author:  fossette
 * 
 * Date:    2015/05/25
 *
 * Version: 1.0
 * 
 * Descr:   Switch between different .vdi disk files of the same virtual machine.
 *          Tested using VirtualBox v4.3.24 under FreeBSD 10.1.
 *          Should be portable because only standard libraries are used.
 * 
 *          Optional parameter: the directory to use.
 *          If not specified, the current directory is used.
 * 
 * Web:     https://github.com/fossette/vbswitch/wiki
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>


/*
 *  Constants
 */

#define LNSZ   256
#define NBSW   20
#define NL     "\n\r"

#define ERROR_VBSWITCH_DIR       1
#define ERROR_VBSWITCH_MEM       2
#define ERROR_VBSWITCH_NBINST    3
#define ERROR_VBSWITCH_NBREF     4
#define ERROR_VBSWITCH_NOREF     5
#define ERROR_VBSWITCH_PREFIX    6
#define ERROR_VBSWITCH_QUIT      99


/*
 *  Types
 */

typedef char FILENAME[LNSZ];


/*
 *  Exist
 */

int
Exist(char *szFilename)
{
  struct stat buffer;
  
  
  return (!stat(szFilename, &buffer));
}


/*
 *  StrGet
 */

void
StrGet(     char *sz)
{
   int i;
   
   
   fgets(sz, LNSZ-1, stdin);
   sz[LNSZ-1] = 0;
   i = strlen(sz);
   while (i)
   {
      i--;
      if (sz[i] == '\n' || sz[i] == '\r')
         sz[i] = 0;
      else
         i = 0;
   }
}


/*
 *  StrnCopy
 */

void
StrnCopy(char *dst, const char *src, const int l)
{
   if (l > 1)
   {
      strncpy(dst, src, l-1);
      dst[l-1] = 0;
   }
   else
      *dst = 0;
}


/*
 *  SplitFilename
 */

void
SplitFilename(const char *szFilename,     char *szBase, char *szExt)
{
   int l;
   
   
   *szBase = 0;
   *szExt = 0;
   l = strlen(szFilename);
   if (l > 2)
   {
      while (l && szFilename[l-1] != '.')
         l--;
      
      if (l)
      {
         StrnCopy(szBase, szFilename, l);
         StrnCopy(szExt, szFilename+l, LNSZ);
      }
   }
}


/*
 *  FileTypeRef
 */

int
FileTypeRef(const char *szFilename)
{
   FILENAME szBase,
            szExt; 
   
   
   SplitFilename(szFilename,     szBase, szExt);   
   return(!strcasecmp(szExt, "vbswitch"));
}


/*
 *  FileTypeVm
 */

int
FileTypeVm(const char *szFilename)
{
   FILENAME szBase,
            szExt; 
   
   
   SplitFilename(szFilename,     szBase, szExt);   
   return(!strcasecmp(szExt, "vdi"));
}


/*
 *  Main
 */

int
main(int argc, char** argv)
{
   int            i,
                  iErr = 0,
                  iNbFilenames = 0,
                  j;
   DIR            *pDir;
   struct dirent  *pEntry;
   FILENAME       *pFilenames = NULL,
                  szBase,
                  szExt,
                  szPath,
                  szPath2,
                  szPrefix,
                  szRefBase,
                  szRefExt,
                  szRefFilename;
    

   // Initialisation
   szRefFilename[0] = 0;
   printf(NL "vbswitch v1.0" NL
             "-------------" NL
             "https://github.com/fossette/vbswitch/wiki" NL NL);
   
   // Option parsing
   if (argc == 2)
      if (argv[1][0])
         chdir(argv[1]);
   
   getcwd(szPath, LNSZ-1);
   szPath[LNSZ-1] = 0;
   printf("Working directory: %s" NL, szPath);
   
   // Load filenames in the selected directory
   pDir = opendir(".");
   if (pDir)
   {
      do
      {
         pEntry = readdir(pDir);
         if (pEntry)
            if (pEntry->d_type == DT_REG)
            {
               if (FileTypeVm(pEntry->d_name))
               {
                  if (iNbFilenames)
                     pFilenames = (FILENAME *)realloc(pFilenames,
                                     (iNbFilenames + 1) * sizeof(FILENAME));
                  else
                     pFilenames = (FILENAME *)malloc(sizeof(FILENAME));
                  if (pFilenames)
                  {
                     StrnCopy(pFilenames[iNbFilenames], pEntry->d_name, LNSZ);
                     iNbFilenames++;
                  }
                  else
                  {
                     iNbFilenames = 0;
                     iErr = ERROR_VBSWITCH_MEM;
                  }
               }
               else if (FileTypeRef(pEntry->d_name))
               {
                  if (szRefFilename[0])
                     iErr = ERROR_VBSWITCH_NBREF;
                  else
                     StrnCopy(szRefFilename, pEntry->d_name, LNSZ);                      
               }
            }
      }
      while (pEntry && !iErr);
      closedir(pDir);
   }
   else
      iErr = ERROR_VBSWITCH_DIR;
       
   if (!iErr && !(szRefFilename[0]))
      iErr = ERROR_VBSWITCH_NOREF;
   if (!iErr && iNbFilenames < 2)
      iErr = ERROR_VBSWITCH_NBINST;
      
   // Find the .vdi files' prefix
   if (!iErr)
   {
      strcpy(szPrefix, szRefFilename);
      i = iNbFilenames;
      while (i)
      {
         i--;
         j = strlen(szPrefix);
         while (j)
         {
            j--;
            if (szPrefix[j] != pFilenames[i][j])
               szPrefix[j] = 0;
         }
      }
      
      if (szPrefix[0])
         printf("Base name:         %s" NL, szPrefix);
      else
         iErr = ERROR_VBSWITCH_PREFIX;
   }
   
   if (!iErr)
   {
      // Show Current .vdi file
      SplitFilename(szRefFilename,     szRefBase, szRefExt);
      printf("Current .vdi file: %s" NL, szRefBase + strlen(szPrefix));
      
      // Show the .vdi file choices
      for (i = 0 ; i < iNbFilenames ; i++)
      {
         SplitFilename(pFilenames[i],     szBase, szExt);
         if (szBase[strlen(szPrefix)])
            printf("              %3d) %s" NL, i+1, szBase + strlen(szPrefix));         
      }
      
      // Request a .vdi file choice
      printf(NL "Choose the .vdi file to switch to ([ENTER]=Quit): ");
      StrGet(szPath);
      i = atoi(szPath) - 1;
      if (i >= 0 && i < iNbFilenames)
      {
         SplitFilename(pFilenames[i],     szBase, szExt);
         if (!szBase[strlen(szPrefix)])
            i = iNbFilenames;
      }
      
      // Switch selected .vdi files
      if (i >= 0 && i < iNbFilenames)
      {
         sprintf(szPath, "%s.%s", szPrefix, szExt);
         sprintf(szPath2, "%s.%s", szRefBase, szExt);
         if (!Exist(szPath2))
         {
            printf("  %s -> %s" NL, szPath, szPath2);
            rename(szPath,     szPath2);
         }
         
         sprintf(szPath2, "%s.%s", szPrefix, szRefExt);
         if (!Exist(szPath2))
         {
            printf("  %s -> %s" NL, szRefFilename, szPath2);
            rename(szRefFilename,     szPath2);
         }
         
         sprintf(szPath2, "%s.%s", szPrefix, szExt);
         if (!Exist(szPath2))
         {
            printf("  %s -> %s" NL, pFilenames[i], szPath2);
            rename(pFilenames[i],     szPath2);
         }
         
         sprintf(szPath, "%s.%s", szPrefix, szRefExt);
         sprintf(szPath2, "%s.%s", szBase, szRefExt);
         if (!Exist(szPath2))
         {
            printf("  %s -> %s" NL, szPath, szPath2);
            rename(szPath,     szPath2);
         }
      }
      else
         iErr = ERROR_VBSWITCH_QUIT;
   }
   
   // Closing
   if (pFilenames)
      free(pFilenames);
   
   switch (iErr)
   {         
      case ERROR_VBSWITCH_DIR:
         printf(NL "ERROR: Specified path can't be accessed." NL);
         break;
         
      case ERROR_VBSWITCH_MEM:
         printf(NL "ERROR: Out of memory." NL);
         break;
         
      case ERROR_VBSWITCH_NBINST:
         printf(NL "ERROR: At least two .vdi files are required to move on." NL);
         break;
         
      case ERROR_VBSWITCH_NBREF:
         printf(NL "ERROR: Can't have more than one reference file." NL);
         break;
         
      case ERROR_VBSWITCH_NOREF:
         printf(NL "ERROR: No .vbswitch reference file found." NL);
         break;
         
      case ERROR_VBSWITCH_PREFIX:
         printf(NL "ERROR: .vdi files do not have a common prefix." NL);
         break;
   }
   
   if (iErr != ERROR_VBSWITCH_QUIT)
   {
      printf(NL "Press ENTER to quit! ");
      StrGet(szPath);
      printf(NL);
   }
    
   return(iErr ? EXIT_FAILURE : EXIT_SUCCESS);
}

