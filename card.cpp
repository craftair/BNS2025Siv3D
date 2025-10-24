# include "card.h"

RectF Card::rect(const SizeF& size) const
{
	return RectF{ Arg::center(center), size };
}
