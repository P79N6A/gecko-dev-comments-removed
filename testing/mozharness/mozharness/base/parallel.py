





"""Generic ways to parallelize jobs.
"""



class ChunkingMixin(object):
    """Generic signing helper methods.
    """
    def query_chunked_list(self, possible_list, this_chunk, total_chunks,
                           sort=False):
        """Split a list of items into a certain number of chunks and
        return the subset of that will occur in this chunk.

        Ported from build.l10n.getLocalesForChunk in build/tools.
        """
        if sort:
            possible_list = sorted(possible_list)
        else:
            
            possible_list = possible_list[:]
        length = len(possible_list)
        for c in range(1, total_chunks + 1):
            n = length / total_chunks
            
            
            if c <= (length % total_chunks):
                n += 1
            if c == this_chunk:
                return possible_list[0:n]
            del possible_list[0:n]
