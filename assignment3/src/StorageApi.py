import logging
import os
import cloudstorage as gcs
import webapp2
import re
import sys
from google.appengine.ext import blobstore
from google.appengine.ext.webapp import blobstore_handlers
import threading
import gc 

class StorageApi(object):
	"""
	Main API that wraps functions from Google Cloud Storage and functions from
	Memcached in order to improve the small size file performance.
	"""
	def __init__(self, bucket_name, memcache_client=None):
		super(StorageApi, self).__init__()
		self.client = memcache_client
		self.bucket_name = '/' + bucket_name + '/'

	# Multiple threads insertion
	def multi_insert(self, blobinfos, nthreads=1):
		threads = [None]*nthreads
		results = []
		blob_lock = threading.Lock()
		for i, th in enumerate(threads):
			threads[i] = threading.Thread(target=self.thread_insert, args=(blobinfos, blob_lock, results))
			threads[i].start()
		for th in threads:
			th.join()
		return results

	def thread_insert(self, blobinfos, blob_lock, results):
		while blobinfos:
			blob_lock.acquire()
			blobinfo = blobinfos.pop()
			blob_lock.release()
			result = self.insert(blobinfo)
			results.append((blobinfo, result))
		return

	# Single thread insertion
	def insert(self, blobinfo):
		"""
		Update the key translator and insert the file to memcache if its size is < 100KB
		"""
		size = blobinfo.size
		# Add to GCS
		key = self.bucket_name + blobinfo.filename
		with gcs.open(key, "w") as gcs_file:
			blob_reader = blobstore.BlobReader(blobinfo)
			value = blob_reader.read(1024)
			while value:
				gcs_file.write(value)
				value = blob_reader.read(1024)
		# Add to the cache if < 100KB
		if size < 100*1024:
			if not self.client:
				logging.warning("Memcached is not activated.")
				return True
			else:
				blob_reader = blobstore.BlobReader(blobinfo)
				value = blob_reader.read()
				if not self.client.set(key,
									   value, time=3600):
					logging.error("Add to memcache did not work.")
					blobstore.delete(blobinfo.key())
					return False

		blobstore.delete(blobinfo.key())
		return True

	def check(self, key):
		"""
		Verify if a file named key exists in the cache or distributed storage system.
		"""
		if not key:
			return False

		if self.check_cache(key):
			return True
		elif self.check_storage(key):
			return True
		else:
			return False

	# Multi threaded find
	def multi_find(self, keys, nthreads=1):
		threads = [None]*nthreads
		results = []
		keys_lock = threading.Lock()
		for i, th in enumerate(threads):
			threads[i] = threading.Thread(target=self.thread_find, args=(keys, keys_lock, results))
			threads[i].start()
		for th in threads:
			th.join()
		return results

	def thread_find(self, keys, keys_lock, results):
		while keys:
			keys_lock.acquire()
			key = keys.pop()
			keys_lock.release()
			found = self.find(key)
			result = found != False
			del(found)
			gc.collect()
			results.append((key, result))
		return

	def find(self, key):
		"""
		Retrieves the contents of a file named key from the cache or the distributed storage system.
		"""
		if not key:
			logging.error("key should be non null")
			return False

		if self.client is not None:
			value = self.client.get(self.bucket_name + key)
			if value is not None:
				return value
		if self.check_storage(key):
			with gcs.open(self.bucket_name + key) as gcs_file:
				content = ""
				value = gcs_file.read(1024)
				while value:
					content += value
					value = gcs_file.read(1024)
			return content
		else:
			return False


	# Multi threaded find
	def multi_remove(self, keys, nthreads=1):
		threads = [None]*nthreads
		results = []
		keys_lock = threading.Lock()
		for i, th in enumerate(threads):
			threads[i] = threading.Thread(target=self.thread_remove, args=(keys, keys_lock, results))
			threads[i].start()
		for th in threads:
			th.join()
		return results

	def thread_remove(self, keys, keys_lock, results):
		while keys:
			keys_lock.acquire()
			key = keys.pop()
			keys_lock.release()
			result = self.remove(key) != False
			results.append((key, result))
		return

	def remove(self, key):
		"""
		Remove the file named key from the cache and the distributed storage system.
		"""
		if not key:
			return False

		removed_key = self.bucket_name + key
		if self.check_cache(key):
			if self.client is not None:
				self.client.delete(removed_key)
		if self.check_storage(key):
			gcs.delete(removed_key)
			return True
		return False

	def listing(self, regex=None):
		"""
		Retrieve a list of all file names as an array.
		"""
		bucket_files = map(lambda x: (x.filename, x.st_size),
						   gcs.listbucket(self.bucket_name))
		if regex is not None:
			return self.listing_regex(bucket_files, regex)
		else:
			return bucket_files

	def check_storage(self, key):
		"""
		Verify if a file named key exists in the storage system.
		"""
		if not key:
			return False

		key = self.bucket_name + key
		if key in [t[0] for t in self.listing()]:
			return True
		else:
			return False

	def check_cache(self, key):
		"""
		Verify if a file named key exists in the cache of the distributed storage system.
		"""
		if not key or not self.client:
			return False

		key = self.bucket_name + key
		files = self.client.get(key)
		if files is not None:
			return True
		else:
			return False

	def remove_all_cache(self):
		"""
		Remove all files from the cache.
		"""
		if not self.client:
			logging.warning("Memcached is not activated.")
			return True

		if self.client.flush_all():
			return True
		else:
			return False

	def remove_all(self):
		"""
		Remove all files from the cache and the distributed storage system.
		"""
		rm_cache = self.remove_all_cache()
		if not rm_cache:
			return False

		bucket_files = self.listing()
		for f in bucket_files:
			try:
  				gcs.delete(f[0])
			except gcs.NotFoundError:
 				return False
		return True

	def cache_size_mb(self):
		"""
		Retrieve the total space (in MB) allocated to files in the cache of distributed storage system.
		"""
		if not self.client:
			return 0
		stats = self.client.get_stats()
		size = stats['bytes']
		size = size/1024.**2
		return size

	def cache_size_elem(self):
		"""
		Retrieve the total number of files in the cache of distributed storage system.
		"""
		if not self.client:
			return 0
		return self.client.get_stats()['items']

	def storage_size_mb(self):
		"""
		Retrieve the total space (in MB) allocated to files in the distributed storage system.
		"""
		objects = gcs.listbucket(self.bucket_name)
		total_size = sum(o.st_size for o in objects)
		return total_size/(1024.**2)

	def storage_size_elem(self):
		"""
		Retrieve the total number of files in the distributed storage system.
		"""
		objects = gcs.listbucket(self.bucket_name)
		count = sum(1 for o in objects)
		return count

	def find_in_file(self, key, string):
		"""
		Searches for a regular expression in file key.
		"""
		if not key or not string:
			return False
		content = self.find(key)
		if content:
			pattern = re.compile(string)
			match = pattern.search(content)
			if match:
				return True
			else:
				return False
		else:
			return False

	def listing_regex(self, keys, string):
		"""
		Retrieve a list of all file names as an array, whose names match the regular expression string.
		"""
		if not keys or not string:
			return False

		pattern = re.compile(string)
		matching_keys = filter(lambda x: pattern.match(x[0]), keys)
		return matching_keys
