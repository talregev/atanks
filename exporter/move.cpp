#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
   int highest, lowest;
   int current;
   char command[256];

   if (argc < 3)
   {
       printf("Usage: move highest_number lowest_number\n\n");
       return 1;
   }

   sscanf(argv[1], "%d", &highest);
   sscanf(argv[2], "%d", &lowest);

   if (highest <= lowest)
   {
      printf("Values are not in right order.\n\n");
      return 1;
   }

   current = highest;
   while (current >= lowest)
   {
      sprintf(command, "mv %d.bmp %d.bmp", current, current + 1);
      system(command);
      current--;
   }

   return 0;
}

