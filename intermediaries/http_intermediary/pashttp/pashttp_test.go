package pashttp

import (
	"encoding/binary"
	"fmt"
	"net"
	"testing"
	"time"
)

type fakeAddr struct {
	n string
	s string
}

func (f fakeAddr) Network() string { return f.n }
func (f fakeAddr) String() string  { return f.s }

type fakePasConn struct {
	received [][]byte
	toSend   [][]byte
	closed   bool
}

func (f *fakePasConn) Read(b []byte) (int, error) {
	if f.closed {
		return 0, fmt.Errorf("connection closed")
	}

	if len(f.toSend) < 1 {
		return 0, fmt.Errorf("no data to send!")
	}

	msg := f.toSend[0]

	if len(b) < len(msg) {
		return len(msg) - len(b), fmt.Errorf("argument buffer is too short!")
	}

	f.toSend = f.toSend[1:]

	for i := range msg {
		b[i] = msg[i]
	}

	return len(msg), nil
}

func (f *fakePasConn) Write(b []byte) (int, error) {
	if f.closed {
		return 0, fmt.Errorf("connection closed")
	}

	f.received = append(f.received, b)
	return len(b), nil
}

func (f *fakePasConn) Close() error {
	if f.closed {
		return fmt.Errorf("connection already closed")
	}

	f.closed = true
	return nil
}

func (f fakePasConn) LocalAddr() net.Addr                { return fakeAddr{} }
func (f fakePasConn) RemoteAddr() net.Addr               { return fakeAddr{} }
func (f fakePasConn) SetDeadline(t time.Time) error      { return nil }
func (f fakePasConn) SetReadDeadline(t time.Time) error  { return nil }
func (f fakePasConn) SetWriteDeadline(t time.Time) error { return nil }

func NewFakePasConn() *fakePasConn {
	return &fakePasConn{
		received: [][]byte{},
		toSend:   [][]byte{},
		closed:   false,
	}
}

func TestWriteToPasSuccess(t *testing.T) {
	const msg = "reticulating-splines"
	buffToWrite := []byte(msg)
	pc := NewFakePasConn()
	hi := HTTPIntermediary{PasConn: pc}
	if err := hi.WriteToPas(buffToWrite); err != nil {
		t.Errorf("WriteToPas(%v) got error %v, want no error", buffToWrite, err)
		return
	}

	if len(pc.received) != 1 {
		t.Errorf("WriteToPas(%v) got %d messages received, wanted 1", buffToWrite, len(pc.received))
		return
	}

	pl := pc.received[0]
	dataLenBuff := pl[:BYTES_IN_DATA_LENGTH]
	data := pl[BYTES_IN_DATA_LENGTH:]
	if string(data) != msg {
		t.Errorf("WriteToPas(%v) got payload data %q, want %q", buffToWrite, string(data), msg)
	}

	gotDataLen, bytesRead := binary.Varint(dataLenBuff)
	if bytesRead <= 0 {
		t.Errorf("WriteToPas(%v) got %d bytes read, want %d", buffToWrite, bytesRead, BYTES_IN_DATA_LENGTH)
	}

	if int(gotDataLen) != len(buffToWrite) {
		t.Errorf("WriteToPas(%v) got payload length of %d, want %d", buffToWrite, gotDataLen, len(buffToWrite))
	}
}
