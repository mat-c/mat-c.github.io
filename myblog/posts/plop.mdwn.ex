[[!tag  tech life linux]]
[[!taglink  foo]]

[[!meta  author="mct"]]
[[!meta  date="20110215"]]
[[!meta  title="priority inversion"]]

void foo() {
mutex_lock
data = data + 4;
mutex_unlock
}


This code seems harmless, but what happen if it is called from thread
with different priority ?

Let's consider 3 thread A (prio 1(low)), B (prio 2), C (prio 3(high))
* A is running and take the lock
* A is preempted by C
* C try to take the lock, but need to wait A release it
* B is running for a long time and prevent A from releasing the mutex.
* C can't run...

A hack to solve that is to boost thread C prio while it got the lock.

But a cleannest way to solve that is to avoid global lock. How ?
By using atomic operation.


======================
Atomic operation doesn't work like mutex.
Mutex do the serialisation in the lock primitive, atomic operation
does it in the commit (release) operation.

This mean both thread can start the operation and the first to finish
will succed, the other one need to retry.

void foo() {
	int data
	do {
		transaction_start
		data = data + 4;
		transaction_commit
	} while (aborted);
}

* A is running and load data
* A is preempted by C
* C load data, modify it, and commit it


=============
What are classical atomic operation

Risc machine got
* swap (old arm core)
* ldrex/strex

Cisc have 
* classical operation on memory (add, sub, ...)
* cmpxch

=== swap
it is very limited because you can store only 2 state
and allow to do only spinlock or ineficient mutex

spin_lock(*lock)
{
	while(swap(lock, 1);
}

spin_unlock(*lock)
{
	*lock = 0;
}

mutex_lock(*lock)
{
	if (swapb(lock8, 1)) {
		while (swap(lock32, 0x101))
			futex_wait
	}
}

mutex_unlock(*lock)
{
	if (swap(lock32, 0) == 0x101) {
		futex_wake
	}
}


==== ldrex/strex
This is a powerfull atomic extension :
ldrex Rd, [Rm] will load in Rd the value at address Rm.

strex Rs, Rd, [Rm] will store at address Rm the value Rd,
if [Rm] wasn't modified since the last ldrex and Rs=1,
otherwise Rs=0


=== cmpxch(*addr, oldval, newval)
if (*addr == oldval) {
  *addr = newval;
  return 0;
}
else {
 return 1;
}


Note that
cmpxch(*addr, oldval, newval) {
int val;
ldrex val, [addr]
if (val == oldval) {
  strex res, newval, [addr]
  if (res)
    return 0;
}
return 1;
}

The reverse is also true but more complicated.

ldrex Rd, [Rm] : save Rd in a storage indexed by Rm

strex Rs, Rd, [Rm] : recover Rd1 form ldrex, Rs = !cmpxch(Rm, Rd1, Rd)

=================
TLS

==================
lock primitive

- spinlock

- mutex

- semaphore

- cond_var


adv

- mb

- flag
