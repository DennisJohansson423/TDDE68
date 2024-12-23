#include "list.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
	struct list_item root;
	root.value = -1;
	root.next = NULL;

	/* Our test cases*/
	append(&root, 3);
	append(&root, 2);
	append(&root, 7);
	print(&root);
	clear(&root);

	prepend(&root, 3);
	prepend(&root, 2);
	prepend(&root, 7);
	print(&root);
	clear(&root);

	input_sorted(&root, 31);
	input_sorted(&root, 11);
	input_sorted(&root, 13);
	print(&root);
	clear(&root);

	clear(&root);
	printf("Passed all tests! \n");
	return 0;
}
