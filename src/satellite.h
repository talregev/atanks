#ifndef SATELLITE_HEADER_FILE__
#define SATELLITE_HEADER_FILE__


#include "environment.h"
#include "globaldata.h"
#include "virtobj.h"

#define SATELLITE_IMAGE 16
#define CHANCE_TO_SHOOT 100


class SATELLITE
  {
  public:
    int x, y;
    int xv, previous_x;
    GLOBALDATA *_global;
    ENVIRONMENT *_env;

    SATELLITE(GLOBALDATA *global, ENVIRONMENT *env);
    ~SATELLITE();
    void Init();
    void Move();
    void Draw(BITMAP *dest);
    void Shoot();

  };

#endif

