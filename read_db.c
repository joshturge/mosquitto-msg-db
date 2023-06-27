/*
 * Copyright (c) 2023 Joshua Turgeon <jturgeon@x1.turgeon.au>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <fcntl.h>
#include <db.h>
#include <string.h>
#include <err.h>

#include "config.h"

int
main(void)
{
	DB *db_con;
	DBT dbk, dbd;
	int ret;

	db_con = dbopen(db_pathname, O_RDWR | O_SHLOCK,
      db_permissions, db_type, NULL);
  if (db_con == NULL)
		err(1, NULL);

	printf("key,payload\n");

	while (1)
	{
		memset(&dbk, 0, sizeof(dbk));
		memset(&dbd, 0, sizeof(dbd));

		ret = db_con->seq(db_con, &dbk, &dbd, R_NEXT);
		if (ret == -1)
			err(1, "seq returned %d", ret);
		else if (ret == 1)
			break;

		((char *)dbd.data)[dbd.size] = 0; // NOTE: null terminate

		printf("%s,%s\n", (char *)dbk.data, (char *)dbd.data);
	}

	return 0;
}
