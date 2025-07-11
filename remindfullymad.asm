;grammar test
fn.num: node new
	range 1
	node parent
	ret

dec.range[1]: '0-9'

_start_: call num
	match


