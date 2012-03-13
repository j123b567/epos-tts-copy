// still mostly in the design phase

class exception {};
	class illegal_data : public exception {};
		class old_style_exc : public exception {};	/* shriek() called */
	class doom : public exception {};	/* wild pointers found and worse   */
