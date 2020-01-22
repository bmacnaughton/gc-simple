{
	"targets": [{
		"target_name" : "gc-minimal",
		"sources"     : [ "src/gc-minimal.cc" ],
		"include_dirs" : [
			"src",
			"<!(node -e \"require('nan')\")"
		]
  }]
}
