#include "floattext.h"

FLOATTEXT::FLOATTEXT (GLOBALDATA *global, ENVIRONMENT *env, char *text, int xpos, int ypos, int color, alignType alignment)
{
      initialise ();
      setEnvironment (env);
      _text = NULL;
      x = (double)xpos;
      y = (double)ypos;
      _current.x = (int)x;
      _current.y = (int)y;
      _current.w = 0;
      _current.h = 0;
      set_pos (xpos, ypos);
      if (text)
        set_text (text);
      else
        set_text(NULL);
      set_color (color);
      _align = alignment;
      _global = global;
      _halfColor = color;
      sway = NO_SWAY;
      original_x = xpos;
      delta_x = 0;
}


FLOATTEXT::~FLOATTEXT()
{
      if (_env)
        {
          /* To be sure to really remove the full text (might fail due to font differences),
             we "enlarge" the text by 25%: */
          if (_current.w)
            {
              _current.x -= (int)((double)_current.w * 1.125);
              _current.w  = (int)((double)_current.w * 1.250);
            }
          if (_current.h)
            {
              _current.y -= (int)((double)_current.h * 1.125);
              _current.h  = (int)((double)_current.h * 1.250);
            }

          switch (_align)
            {
            case LEFT:
              _env->make_bgupdate (_current.x, _current.y, _current.w, _current.h);
              _env->make_bgupdate (_old.x, _old.y, _old.w, _old.h);
              break;
            case RIGHT:
              _env->make_bgupdate (_current.x - _current.w, _current.y - _current.h, _current.w, _current.h);
              _env->make_bgupdate (_old.x - _old.w, _old.y - _old.h, _old.w, _old.h);
              break;
            case CENTRE:
              _env->make_bgupdate (_current.x - (_current.w / 2), _current.y - (_current.h / 2), _current.w + 2, _current.h + 2);
              _env->make_bgupdate (_old.x - (_old.w / 2), _old.y - (_old.h / 2), _old.w + 2, _old.h + 2);
              break;
            default:
              _env->make_bgupdate (_current.x - _current.w, _current.y - _current.h, 2 * _current.w, 2 * _current.h);
              _env->make_bgupdate (_old.x - _old.w, _old.y - _old.h, 2 * _old.w, 2 * _old.h);
            }
          _env->removeObject (this);
        }
      _global = NULL;
      _env    = NULL;
      if (_text)
      {
         free(_text);
         _text = NULL;
      }
}


int FLOATTEXT::applyPhysics()
{
      VIRTUAL_OBJECT::applyPhysics ();

      if ( (!delta_x) && (sway) )
         delta_x = -1;

      x += delta_x;
      if (x < original_x - sway)
         delta_x = -delta_x;
      else if (x > original_x + sway)
         delta_x = -delta_x;

      set_pos ((int)x, (int)y);

      age++;
      if ( (maxAge != -1) && (age > maxAge) )
        destroy = TRUE;

      return (0);
}


void FLOATTEXT::setEnvironment(ENVIRONMENT *env)
{
      if (!_env || (_env != env))
        {
          _env = env;
          _index = _env->addObject (this);
        }
}     



void FLOATTEXT::draw(BITMAP *dest)
{
      // If there is no text then we have nothing to draw
      if (! _text)
         return;

      if (_current.w)
        {
          double dShadFade		= 0.75;
          int iFrontCol				=	_color;
          int iShadCol				=	_halfColor;
          int iBackCol				=	_color;

          // get Background color:
          if (  (_env->dShadowedText > 0.0) || (_env->dFadingText > 0.0)  )
          {
            switch (_align)
              {
              case LEFT:
                iBackCol = _env->getAvgBgColor(	_current.x,								_current.y,
                                                _current.x + _current.w,	_current.y + _current.h,
                                                xv,												yv);
                break;
              case RIGHT:
                iBackCol = _env->getAvgBgColor(	_current.x - _current.w,	_current.y - _current.h,
                                                _current.x,								_current.y,
                                                xv,												yv);
                break;
              case CENTRE:
                iBackCol = _env->getAvgBgColor(	_current.x - (_current.w / 2),		_current.y - (_current.h / 2),
                                                _current.x + (_current.w / 2) + 2,_current.y + (_current.h / 2) + 2,
                                                xv,																yv);
                break;
              }
          }
          if ((maxAge > 0) && (age >= (maxAge / 2)) && (_env->dFadingText > 0.0))
            {
              double dCalcAge = (double)age - (double)(maxAge / 2);
              double dCalcMax = (double)(maxAge / 2);
              double dFrontFade	= 1.0 - (dCalcAge / dCalcMax);
              dShadFade		=	0.75 - (0.75 * (dCalcAge / dCalcMax));
              if (dFrontFade	< 0.0) dFrontFade		= 0.0;
              if (dFrontFade	> 1.0) dFrontFade		= 1.0;
              if (dShadFade < 0.0) dShadFade	= 0.0;
              if (dShadFade >	1.0) dShadFade	= 1.0;
              iFrontCol = newmakecol((getr(iFrontCol) * dFrontFade) + (getr(iBackCol) * (1.0 - dFrontFade)),
                                  (getg(iFrontCol) * dFrontFade) + (getg(iBackCol) * (1.0 - dFrontFade)),
                                  (getb(iFrontCol) * dFrontFade) + (getb(iBackCol) * (1.0 - dFrontFade)));
            }
          if (_env->dShadowedText > 0.0)
            iShadCol = newmakecol(	(getr(iShadCol) * dShadFade) + (getr(iBackCol) * (1.0 - dShadFade)),
                                (getg(iShadCol) * dShadFade) + (getg(iBackCol) * (1.0 - dShadFade)),
                                (getb(iShadCol) * dShadFade) + (getb(iBackCol) * (1.0 - dShadFade)));
          if (_align == LEFT)
            {
              if (_env->dShadowedText > 0.0)
                textout_ex (dest, font, _text,
                            _current.x + 1, _current.y + 1, iShadCol, -1);
              textout_ex (dest, font, _text,
                          _current.x, _current.y, iFrontCol, -1);
            }
          else if (_align == RIGHT)
            {
              if (_env->dShadowedText > 0.0)
                textout_ex (dest, font, _text,
                            _current.x - _current.w + 1,
                            _current.y - _current.h + 1, iShadCol, -1);
              textout_ex (dest, font, _text,
                          _current.x - _current.w,
                          _current.y - _current.h, iFrontCol, -1);
            }
          else
            {
              if (_env->dShadowedText > 0.0)
                textout_centre_ex (dest, font, _text,
                                   _current.x + 1, _current.y + 1, iShadCol, -1);
              textout_centre_ex (dest, font, _text,
                                 _current.x, _current.y, iFrontCol, -1);
            }
        }
}



// Returns TRUE on success or FALSE on failure.
int FLOATTEXT::set_text(char *text)
{
   // int my_length = strlen(text);
   int my_length;

   // clean up old text
   if (_text)
   {
      free(_text);
      _text = NULL;
   }
   if (! text)
      return TRUE;

   my_length = strlen(text);
   _text = (char *) calloc( my_length + 1, sizeof(char));
   if (! _text)
      return FALSE;

   strcpy(_text, text);
   if (my_length)
        _current.w = text_length(font, _text) + 3;
   else
       _current.w = 0;
   _current.h = text_height(font) + 15;
   return TRUE;
}


void FLOATTEXT::set_color(int color)
{
      int red		=	getr(color);
      int green	=	getg(color);
      int blue	=	getb(color);
      int iAverage = (red + green + blue) / 3;

      _color = color;

      if (!(red & 0xc0) && !(green & 0xc0) && !(blue & 0xc0))
        _halfColor = makecol(red | 0x80, green | 0x80, blue | 0x80); // Color is too dark, shadow should be bright
      else
        _halfColor = makecol(	red		> iAverage?(red		/ 4):(red		/ 6),
                              green	> iAverage?(green	/ 4):(green	/ 6),
                              blue	> iAverage?(blue	/ 4):(blue	/ 6));
}


void FLOATTEXT::newRound()
{
   age = maxAge + 1;
}

void FLOATTEXT::set_pos(int x, int y)
{
   _current.x = x;
   _current.y = y;
}



void FLOATTEXT::set_speed(double x_speed, double y_speed)
{
     double mix_it_up;       // avoid over-lapping text

     mix_it_up = rand() % SPEED_RANGE;      // (0-5)
     mix_it_up -= 3.0;                      // -3 to +2
     mix_it_up /= 10.0;                     // -0.3 to + 0.2
     xv = x_speed;
     yv = y_speed + mix_it_up;
     if ( (yv == 0.0) && (y_speed < 0.0) )
        yv = -0.1;       // avoid text that does not move
}

