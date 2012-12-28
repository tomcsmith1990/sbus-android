package uk.ac.cam.tcs40.sbus.airs.sensor.dynamic;

public class SafeMessageQueue<T> implements MessageQueue<T> {
	private static class Link<L> {
		L val;
		Link<L> next;
		Link(L val) { this.val = val; this.next = null; }
	}
	private Link<T> first = null;
	private Link<T> last = null;

	public void put(T val) {
		synchronized(this) {
			//	given a new "val", create a new Link<T>
			//  element to contain it and update "first" and
			//  "last" as appropriate
			Link<T> link = new Link<T>(val);

			if (first == null) first = link;

			if (last != null) last.next = link;

			last = link;
			
			this.notify();
		}
	}

	public T take() {
		synchronized(this) {
			while(first == null) //use a loop to block thread until data is available
				try {this.wait();} catch(InterruptedException ie) {}
				//	retrieve "val" from "first", update "first" to refer
				//  to next element in list (if any). Return "val"
				T val = first.val;
				first = first.next;
				return val;
		}
	}
}