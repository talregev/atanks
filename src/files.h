#ifndef FILE_HANDLING_HEADER_
#define FILE_HANDLING_HEADER_

#define MAX_CONFIG_LINE 128
#define MAX_INSULAND_LINE 256

#include "debug.h"

#ifndef HAS_DIRENT
#  if defined(ATANKS_IS_MSVC)
#    include "extern/dirent.h"
#  else
#    include <dirent.h>
#  endif // Linux
#  define HAS_DIRENT 1
#endif //HAS_DIRENT

#include "globaldata.h"
#include "environment.h"
#include "text.h"

/* Global path buffer */
extern char path_buf[PATH_MAX + 1];


bool Save_Game();
bool Load_Game();
bool Check_For_Saved_Game();
bool Copy_Config_File();


// Make sure there is a music folder in .atanks
bool Create_Music_Folder();
void scrollTextList(TEXTBLOCK* lines);
void flush_inputs();
bool Load_Weapons_Text();


#ifdef MACOSX
  int Filter_File( struct dirent *my_file );
#else
  int Filter_File( const struct dirent *my_file );
#endif

dirent** Find_Saved_Games(uint32_t &num_files_found);

char** Find_Bitmaps(int32_t* bitmaps_found);

#endif
