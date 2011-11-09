/*
 * tl2796.h -- Platform glue for Samsung tl2796 AMOLED panel
 *
 * Author: Joerie de Gram <j.de.gram@gmail.com>
 * Based on l4f00242t03.h by Alberto Panizzo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _INCLUDE_LINUX_SPI_TL2796_H_
#define _INCLUDE_LINUX_SPI_TL2796_H_

struct tl2796_platform_data {
	unsigned int	reset_gpio;
	unsigned int	reg_reset_gpio;
	const char 	*io_supply;	/* will be set to 1.8 V */
	const char 	*core_supply;	/* will be set to 3.15 V */
	const char 	*pll_supply;	/* will be set to 1.8 V */
};

#endif /* _INCLUDE_LINUX_SPI_TL2796_H_ */

