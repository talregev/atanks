#ifndef SATELLITE_HEADER_FILE__
#define SATELLITE_HEADER_FILE__


#include "environment.h"
#include "globaldata.h"

#define SATELLITE_IMAGE 16
#define CHANCE_TO_SHOOT 100

#ifndef BEAM_DEFINE
class BEAM;
#endif // BEAM_DEFINE

class SATELLITE
{
public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit SATELLITE();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void draw();
	void move();
	void shoot();


private:

	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	BEAM*   beam   = nullptr;
	int32_t x      = 0;
	int32_t y      = MENUHEIGHT + 5;
	int32_t xv     = -2;
	int32_t prev_x = 0;

};

#endif

