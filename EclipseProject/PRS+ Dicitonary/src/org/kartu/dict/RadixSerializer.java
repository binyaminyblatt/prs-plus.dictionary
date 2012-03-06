package org.kartu.dict;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.List;

import org.kartu.IOUtils;

import ds.tree.RadixTreeImpl;
import ds.tree.RadixTreeNode;

public class RadixSerializer {
	private static final String ENCODING = "UTF8";

	public static void main(String[] args) throws IOException {
		RandomAccessFile raf = new RandomAccessFile("radix.dat", "rw");
		raf.setLength(0);
		RadixTreeImpl<Integer> radixTree = new RadixTreeImpl<Integer>();
		radixTree.insert("one", 1);
		radixTree.insert("on", 2);
		radixTree.insert("once", 3);
		persistRadix(0, radixTree.root, raf); 
		raf.close();
	}
	
	public static final int persistRadix(int offset, RadixTreeNode<Integer> node, RandomAccessFile out) throws IOException {
		// size of the block (short)
		// value (VALUE_SIZE), 0 if not real
		// num of children (byte)
		// *children
		// children names
		
		int initialOffset = offset;
		int size = 0;
		
		// size of the block (short)
		size += IOUtils.writeShort(out, 0);
		
		// value
		size += IOUtils.writeInt(out, node.real ? node.value : 0);

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
		for (RadixTreeNode<Integer> child : node.childern) {
			byte[] buf = child.key.getBytes(ENCODING);
			out.write(buf);
			out.write(0);
			size += buf.length + 1;
		}
		
		// fill children pointers
		offset += size;
		List<Integer> childrenOffsets = new ArrayList<Integer>(node.childern.size());
		for (RadixTreeNode<Integer> child : node.childern) {
			childrenOffsets.add(offset);
			offset = persistRadix(offset, child, out);
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
