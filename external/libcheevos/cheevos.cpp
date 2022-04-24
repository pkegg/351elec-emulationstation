#include "cheevos.h"
#include "rcheevos/include/rc_hash.h"
#include "rcheevos/include/rc_consoles.h"
#include "libretro-common/include/formats/cdfs.h"
#include "libretro.h"

#include <cstdlib>
#include <memory>

#define CHEEVOS_FREE(p) do { void* q = (void*)p; if (q) std::free(q); } while (0)

static void* rc_hash_handle_cd_open_track(const char* path, uint32_t track)
{
	cdfs_track_t* cdfs_track;

	if (track == 0)
		cdfs_track = cdfs_open_data_track(path);
	else
		cdfs_track = cdfs_open_track(path, track);

	if (cdfs_track)
	{
		cdfs_file_t* file = (cdfs_file_t*)std::malloc(sizeof(cdfs_file_t));
		if (cdfs_open_file(file, cdfs_track, NULL))
			return file;

		CHEEVOS_FREE(file);
	}

	cdfs_close_track(cdfs_track); /* ASSERT: this free()s cdfs_track */
	return NULL;
}

static size_t rc_hash_handle_cd_read_sector(void* track_handle, uint32_t sector, void* buffer, size_t requested_bytes)
{
	cdfs_file_t* file = (cdfs_file_t*)track_handle;

	uint32_t track_sectors = cdfs_get_num_sectors(file);

	sector -= cdfs_get_first_sector(file);
	if (sector >= track_sectors)
		return 0;

	cdfs_seek_sector(file, sector);
	return cdfs_read_file(file, buffer, requested_bytes);
}

static void rc_hash_handle_cd_close_track(void* track_handle)
{
	cdfs_file_t* file = (cdfs_file_t*)track_handle;
	if (file)
	{
		cdfs_close_track(file->track);
		cdfs_close_file(file); /* ASSERT: this does not free() file */
		CHEEVOS_FREE(file);
	}
}

static uint32_t rc_hash_handle_cd_first_track_sector(void* track_handle)
{
	cdfs_file_t* file = (cdfs_file_t*)track_handle;
	return cdfs_get_first_sector(file);
}

static rc_hash_filereader filereader;
static rc_hash_cdreader cdreader;
static bool readersinit = false;

bool generateHashFromFile(char hash[33], int console_id, const char* path)
{
	try
	{
		if (!readersinit)
		{
			cdreader.open_track = rc_hash_handle_cd_open_track;
			cdreader.read_sector = rc_hash_handle_cd_read_sector;
			cdreader.close_track = rc_hash_handle_cd_close_track;
			cdreader.first_track_sector = rc_hash_handle_cd_first_track_sector;
//			cdreader.absolute_sector_to_track_sector = nullptr;
			rc_hash_init_custom_cdreader(&cdreader);

			readersinit = true;
		}

		return rc_hash_generate_from_file(hash, console_id, path) != 0;
	}
	catch (...)
	{
	}

	return false;
}
