package org.kartu.dict;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;

import org.kartu.IOUtils;

import ds.tree.RadixTreeNode;

/**
 * Class that can create prsp dictionary files from Radix trees
 * 
 * @author kartu
 */
public class RadixSerializer {
	// UTF16 is used for key encoding, to simplify case/accent insensitive lookup
	// UTF8 adds var-length complexity
	// The fact that UTF-16 is formally var-length is ignored.
	private static final RadixSerializer instance = new RadixSerializer();
	// End of string marker, 2 zero bytes
	private static final byte[] EOSTR = new byte[2];
	
	public static RadixSerializer getInstance() {
		return instance;
	}

	// Persist word list + pointers
	public final int persistRadix(Charset charset, int offset, int offsetValue1, int offsetValue2, RadixTreeNode<int[]> node, RandomAccessFile out) throws IOException {
		// size of the block (short)
		// value (VALUE_SIZE), 0 if not real
		// num of children (byte) FIXME for Chinese might need to change to short
		// *children
		// children names
		
		int initialOffset = offset;
		int size = 0;
		
		// size of the block (short)
		size += IOUtils.writeShort(out, 0);
		
		// value
		size += IOUtils.writeInt(out, node.real ? node.value[0] + offsetValue1 : 0);
		size += IOUtils.writeInt(out, node.real ? node.value[1] + offsetValue2 : 0);

		// num of children
		int nChildren = node.childern.size();
		out.write(nChildren);
		size += 1;
		
		// children pointers
		int childrenPOffset = offset + size;
		for (int i = 0; i < nChildren; i++) {
			// fill with zeros
			size += IOUtils.writeInt(out, 0);
		}

		// children names (heading length)
		for (RadixTreeNode<int[]> child : node.childern) {
			byte[] buf = child.key.getBytes(charset);
			out.write(buf);
			out.write(EOSTR);
			size += buf.length + EOSTR.length;
		}
		
		// fill children pointers
		offset += size;
		List<Integer> childrenOffsets = new ArrayList<Integer>(node.childern.size());
		for (RadixTreeNode<int[]> child : node.childern) {
			childrenOffsets.add(offset);
			offset = persistRadix(charset, offset, offsetValue1, offsetValue2, child, out);
		}
		
		// Write size of the block
		out.seek(initialOffset);
		IOUtils.writeShort(out, size);
		
		// Write children pointers
		out.seek(childrenPOffset);
		for (int off : childrenOffsets) {
			IOUtils.writeInt(out, off);
		}
		
		// Set file pointer back (offset contains pointer to the next element)
		out.seek(offset);
		
		return offset;
	}
}
