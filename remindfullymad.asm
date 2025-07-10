;grammar test

dec.range[1]: '0-9'

fn.num: node new
	range 1
	node parent
	ret

_start_: call num
	match


