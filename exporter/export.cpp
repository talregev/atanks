#include <allegro.h>
#include <stdio.h>

/*
 * Compile with the line g++ -o Export export.cpp `allegro-config --libs`
 *
 * This program extracts all bitmaps from an Allegro datafile.
*/

int main(int argc, char *argv[])
{
   int counter = 0;
   char filename[256];
   BITMAP *new_bitmap;
   DATAFILE *my_datafile;

   if (argc < 2)
   {
      printf("usage: export datafile\n");
      return 0;
   }

   allegro_init();
   set_color_depth(24);

   // load a datafile
   my_datafile = load_datafile(argv[1]);
   if (! my_datafile)
   {
        printf("Unable to open datafile.\n");
        return 1;
   }

   // for each bitmap, save to a new file
   while (counter < 256)
   {
      new_bitmap = (BITMAP *) my_datafile[counter].dat;
      if (new_bitmap)
      {
        sprintf(filename, "%d.bmp", counter);
        save_bmp(filename, new_bitmap, NULL);
      }
      counter++;
   }

   unload_datafile(my_datafile);
   return 0;
}

