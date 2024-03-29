package rdpmctsc

//go:noescape
func rdPmcTsc() uint64

func RdPmcTsc() (res uint64) {
	res = rdPmcTsc()
	return res
}
