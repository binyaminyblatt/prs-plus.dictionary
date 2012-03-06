package org.kartu.xdxf;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.Collections;

import javax.xml.bind.JAXBException;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamReader;

import org.kartu.dict.RadixSerializer;
import org.kartu.dict.xdxf.xml.AR;

import ds.tree.DuplicateKeyException;
import ds.tree.RadixTreeImpl;

// TODO delete this class (obsolete)
public class XDXFParser {
	private static final String ENCODING = "UTF8";
	public static void main(String[] args) throws XMLStreamException, JAXBException, IOException {

		PrintWriter pw = new PrintWriter("articles.dat", "UTF8");
		
		// StaX
		XMLInputFactory f = XMLInputFactory.newInstance();
		f.setProperty("javax.xml.stream.supportDTD", false);
		f.setProperty("javax.xml.stream.isSupportingExternalEntities", false);
		FileInputStream fin = new FileInputStream("eng-rus.xdxf");
		XMLStreamReader r = f.createXMLStreamReader(fin);

		RandomAccessFile words = new RandomAccessFile("words.dat", "rw");
		
		RadixTreeImpl<Integer> radixTree = new RadixTreeImpl<Integer>();
		int nWordCount = 0;
		ArrayList<String> wordList = new ArrayList<String> (50000);
		while(r.hasNext()) {
		    r.next();
		    if (r.isStartElement()) {
		    	String tagName = r.getLocalName();
		    	if ("ar".equals(tagName)) {
		    		nWordCount++;
		    		AR ar = (AR) AR.unmarshaller.unmarshal(r);
		    		pw.write(ar.toString());
		    		
		    		for (String key : ar.getKeywords()) {
		    			//System.out.println(key);
		    			//trie.put(key, ar);
		    			words.write(key.getBytes(ENCODING));
		    			words.write('\n');
		    			wordList.add(key);
		    		}
		    	} else if ("xdxf".equals(tagName)) {
		    		System.out.println("Lang from: " + r.getAttributeValue(null, "lang_from"));
		    		System.out.println("Lang to: " + r.getAttributeValue(null, "lang_to"));
		    		System.out.println("Format: " + r.getAttributeValue(null, "format"));
		    	} else if ("full_name".equals(tagName)) {
		    		System.out.println("Dict name: " + r.getElementText());
		    	} 
		    }
		}
		words.close();
		pw.close();
		
		Collections.sort(wordList);
		nWordCount = 0;
		for (String word : wordList) {
			try {
				radixTree.insert(word, nWordCount++);
			} catch (DuplicateKeyException dke) {
				System.out.println("Duplicate key (ignoring second value): "  + word);
			}
		}
		
		System.out.println("Word count: " + nWordCount);
		RandomAccessFile raf = new RandomAccessFile("radix.dat", "rw");
		raf.setLength(0);		
		RadixSerializer.persistRadix(0, radixTree.root, raf);
		raf.close();
		/*
		trie.traverse(new Cursor<String, AR>() {
			@Override
			public org.ardverk.collection.Cursor.Decision select(
					Entry<? extends String, ? extends AR> entry) {
				TrieEntry<String, AR> trieEntry = (TrieEntry<String, AR>) entry;
				System.out.println(trieEntry.getKey());
				return Decision.CONTINUE;
			}
		}); */
		
		/*
		 * bitIndex (unsigned byte)
		 * key (string)
		 * left (pointer)
		 * right (pointer)
		 * value (pointer) 
		 */
	}
	

}
