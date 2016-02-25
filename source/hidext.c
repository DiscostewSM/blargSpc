#include <3ds.h>
#include "hidext.h"

static u32 keys = 0;
static u32 keysold = 0;
static u32 keysrepeat = 0;


static u8 delay = 30, repeat = 15, count = 30;

u32 hidKeysDownRepeat(void)
{
	keysold = keys;
	keys = hidKeysHeld();

	if(delay != 0)
	{
		if(keys != keysold)
		{
			count = delay;
			keysrepeat = hidKeysDown();
		}

		count--;
		if(count == 0)
		{
			count = repeat;
			keysrepeat = keys;
		}
	}

	u32 tmp = keysrepeat;
	keysrepeat = 0;
	return tmp;
}

void hidKeysSetRepeat(u8 setDelay, u8 setRepeat)
{
	delay = setDelay;
	repeat = setRepeat;
	count = delay;
	keysrepeat = 0;
}
