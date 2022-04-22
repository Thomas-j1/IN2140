
runAll: create1 create2 create3 load1 load2 load3

create1:
	$(info )
	$(info )
	$(info Running test: create_example1 -----------------------------)
	cd create_example1 && make

create2:
	$(info Finished test: create_example1 -----------------------------)
	$(info )
	$(info )
	$(info Running test: create_example2 -----------------------------)
	cd create_example2 && make

create3:
	$(info Finished test: create_example2 -----------------------------)
	$(info )
	$(info )
	$(info Running test: create_example3 -----------------------------)
	cd create_example3 && make

load1:
	$(info Finished test: create_example3 -----------------------------)
	$(info )
	$(info )
	$(info Running test: load_example1 -----------------------------)
	cd load_example1 && make

load2:
	$(info Finished test: load_example1 -----------------------------)
	$(info )
	$(info )
	$(info Running test: load_example2 -----------------------------)
	cd load_example2 && make

load3:
	$(info Finished test: load_example2 -----------------------------)
	$(info )
	$(info )
	$(info Running test: load_example3 -----------------------------)
	cd load_example3 && make

clean:
	cd load_example1 && make clean
	cd load_example2 && make clean
	cd load_example3 && make clean
	cd create_example1 && make clean
	cd create_example2 && make clean
	cd create_example3 && make clean
